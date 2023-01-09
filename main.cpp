#include "Base64.h"
#include "Utils.h"

// 테서랙트 OCR 이용을 위한 헤더 추가
#include <leptonica/allheaders.h>
#include <tesseract/baseapi.h>
#include <cpr/cpr.h> // CURL을 이용한 가벼운 HTTP POST GET 라이브러리
#include <nlohmann/json.hpp> // JSON
#include <opencv2/opencv.hpp> // OpenCV

namespace json = nlohmann; // json 라이브러리중 가장 성능좋다고 생각하는 nlohmann 라이브러리를 사용하겠습니다.
//tesseract::TessBaseAPI* api = new tesseract::TessBaseAPI(); // 테서렉트 API 사용 클래스 정의

// OCR 방식
//#define TESSERACT_AND_OPENCV_METHOD // 주석 풀시 테서랙트+OpenCV, 주석시 클로바 OCR REST API 사용
// 한글 OCR은 클로바가 압도적으로 잘 읽습니다. ( 부분유료 )
// 2006년부터 개발된 가장 유명한 오픈소스 OCR 라이브러리 -> 태서랙트 : 한글읽기 좀 빡셈 ( 무료 )


// 사용자가 수정해야할 것들
// ***********************************************
#define APIGW_INVOKE_URL "https://5hew6imuyh.apigw.ntruss.com/custom/v1/18892/4703a7e7e66f12451fce6c706327b3cbbcaa35e18b8d084cb3ff39486f84d8ea/general"
#define X_OCR_SECRET_KEY "alBBZnlYeGFxZHJFenNKenFPUU9WV0N6Z2VmdkR2UXI="
// ***********************************************



// OpenCV 함수 사용을 위한 전역변수입니다.
int dilation_elem = 0;
int dilation_size = 0;

__declspec(dllexport) auto read_ocr_words(std::string image_path, std::string data_path) -> std::string {
#ifdef TESSERACT_AND_OPENCV_METHOD

    // 테서렉트 한국어 데이터를 Init, OEM 설정을 tesseract::OcrEngineMode::OEM_TESSERACT_LSTM_COMBINED 로 할시 인식률 증가
    // OEM_TESSERACT_LSTM_COMBINED 쓸려면 kor2 데이터 필요
    if (api->Init(data_path.c_str(), "kor2", tesseract::OcrEngineMode::OEM_TESSERACT_LSTM_COMBINED))
        return "NO_INIT"; // 실패시 실패문자열 반환

    // kor : 기본 문자데이터
    // kor2 : 한글을 좀 더 러닝한 데이터

    //인식률을 높이기 위해 opencv로 이미지를 다듬습니다.
    cv::Mat im = cv::imread(image_path, cv::IMREAD_COLOR);
    cv::resize(im, im, cv::Size(0, 0), 2, 2);
    
    cv::Mat newframe;
    cv::fastNlMeansDenoisingColored(im, newframe, 25, 25, 7, 21);

    int dilation_type;
    if (dilation_elem == 0) { dilation_type = cv::MorphShapes::MORPH_RECT; }
    else if (dilation_elem == 1) { dilation_type = cv::MorphShapes::MORPH_CROSS; }
    else if (dilation_elem == 2) { dilation_type = cv::MorphShapes::MORPH_ELLIPSE; }

    cv::Mat element = cv::getStructuringElement(dilation_type,
        cv::Size(2 * dilation_size + 1, 2 * dilation_size + 1),
        cv::Point(dilation_size, dilation_size));

    cv::Mat newone;
    dilate(newframe, newone, element);

    float alpha = 1.f;
    newone = newone + (newone - 128) * alpha;
    cv::GaussianBlur(newone, newone, cv::Size(7, 7), 1);
    api->SetImage(newone.data, newone.cols, newone.rows, 3, newone.step);
    
    // 이미지 디버그용
    //cv::imshow("1", newone);
    //cv::waitKey(1);

    return api->GetUTF8Text(); // 테서렉트 api사용해서 텍스트 UTF8로 읽음

    // 테서랙트 라이브러리에 내장된 이미지 불러오는기능인데 인식률이 낮아서 사용안합니다.
    //Pix* image = pixRead(image_path.c_str()); // OCR 이미지 정의 및 읽기
    //api->SetImage(image); // OCR 이미지 설정 
#else
    // 클로바 OCR
    unsigned char* test;
    int size;
    Utils::ReadBufferData(image_path, &test, &size);

    json::json request_json;
    request_json["lang"] = "ko"; // 한국어 OCR
    request_json["requestId"] = "string";
    request_json["resultType"] = "string";
    request_json["timestamp"] = 0; // 0으로 해도 받을땐 제대로 옴.
    request_json["version"] = "V2"; // V1보다는 V2가 더 인식률이 좋고 괜찮다고 클로바 docs에 적혀져있었습니당. ㅇㅅㅇ
    request_json["images"][0]["format"] = "jpg"; // 이미지 포맷 png, jpg, jpeg, pdf 지원한다고 적혀져있음.
    request_json["images"][0]["name"] = "medium"; // 이거는 이미지 이름인데 아무렇게나 보내면되겠습니다.
    request_json["images"][0]["data"] = base64->base64_decode(base64->base64_encode(test, size)); // 내 컴퓨터 경로에 있는 파일 바이너리 데이터로 불러와서 base64로 전송.
    //request_json["images"][0]["url"] = "https://imgur.com/PYWUoQI.jpg"; Url에 있는 이미지 이용할때

    //std::cout << request_json.dump() << std::endl;

    auto response = cpr::Post(
        cpr::Url{ APIGW_INVOKE_URL }, // APIGW Invoke URL
        cpr::Header{ {"Content-Type", "application/json"},
        {"X-OCR-SECRET", X_OCR_SECRET_KEY } }, // Secret Key
        cpr::Body{ { request_json.dump() } });

    //std::cout << response.status_code << std::endl;
    //std::cout << Utils::UTF8ToANSI(response.text.c_str()) << std::endl;

    auto rs = json::json::parse(response.text.c_str());

    //std::cout << rs["timestamp"].get<__int64>() << std::endl;
    //std::cout << rs["images"][0]["fields"][0]["inferConfidence"].get<float>() << std::endl;

    //std::cout << Utils::UTF8ToANSI(rs["images"][0]["fields"][1]["inferText"].get<std::string>().c_str()) << std::endl;
    //std::cout << Utils::UTF8ToANSI(rs["images"][0]["fields"][2]["inferText"].get<std::string>().c_str()) << std::endl;
    //std::cout << Utils::UTF8ToANSI(rs["images"][0]["fields"][3]["inferText"].get<std::string>().c_str()) << std::endl;

    std::string result_word = " ";
    for (int i = 0; i < rs["images"][0]["fields"].size(); i++) {
        auto accurate = rs["images"][0]["fields"][i]["inferConfidence"].get<float>();
        if (accurate < 0.8f) {
            //std::cout << "인식률이 나락입니다.\n" << std::endl;
        }

        //std::cout << Utils::UTF8ToANSI(rs["images"][0]["fields"][i]["inferText"].get<std::string>().c_str()) << std::endl;
        result_word = result_word + " " + std::string(Utils::UTF8ToANSI(rs["images"][0]["fields"][i]["inferText"].get<std::string>().c_str()));
    }
    //result_word.erase(std::remove(result_word.begin(), result_word.end(), '')); // 특정 문자 삭제
    //std::cout << result_word << std::endl;

    return result_word;
#endif
}

auto check_spacing_rule(std::string src) -> std::string {
    // 띄어쓰기 검사시작
    auto spacing_rule = cpr::Post(
        cpr::Url({ "http://nlp.kookmin.ac.kr/cgi-bin/asp.cgi" }), cpr::Payload{ {"Question", std::string(Utils::UTF8ToANSI(src.c_str())) } }); // 성능좋은 부산대학교 맞춤법 검사기 사이트를 사용합니다.

    std::size_t bp = spacing_rule.text.rfind("<b>출력"); // json 형태가 될때까지 문자열을 잘라줍니다 1
    std::size_t ep = spacing_rule.text.rfind("</b></td>"); // json 형태가 될때까지 문자열을 잘라줍니다 2
    return spacing_rule.text.substr(bp + 9, ep - bp - 9);
}

__declspec(dllexport) auto check_matchumrule(std::string texts, bool debug_strings) -> bool {
    try {
#ifdef TESSERACT_PLUS_OPENCV_METHOD
        std::string spaced_texts = check_spacing_rule(texts);
        std::cout << spaced_texts << std::endl;
#endif

        //std::cout << texts << std::endl;

        // 맞춤법검사 시작
        auto response = cpr::Post(
            cpr::Url({ "http://164.125.7.61/speller/results" }), cpr::Parameters{ {"text1", std::string(Utils::ANSItoUTF8(texts.c_str())) } }); // 성능좋은 부산대학교 맞춤법 검사기 사이트를 사용합니다.
        // 맞춤법 검사할때는 UTF8로 변환해서 문자열을 보내줘야합니다.

        //std::cout << Utils::UTF8ToANSI(response.text.c_str()) << std::endl;
        if (response.status_code == cpr::status::HTTP_OK) { // HTTP REQUEST 200 - HTTP OK
            if (std::string(Utils::UTF8ToANSI(response.text.c_str())).find("기술적 한계") != std::string::npos) { // 오류를 찾지 못했다면
                std::cout << "오류를 찾지 못했습니다." << std::endl;
                return true; // 맞춤법이 올바름을 리턴합니다.
            }
            else {
                std::size_t begin_pos = response.text.rfind("data = ["); // json 형태가 될때까지 문자열을 잘라줍니다 1
                std::size_t end_pos = response.text.rfind("}];"); // json 형태가 될때까지 문자열을 잘라줍니다 2
                auto temp_filename = response.text.substr(begin_pos, end_pos); // json 형태가 될때까지 문자열을 잘라줍니다 3
                auto filename = temp_filename.substr(0, temp_filename.rfind("}];") - 8); // json 형태가 될때까지 문자열을 잘라줍니다 4
                //std::cout << Utils::UTF8ToANSI(filename.c_str()) << std::endl; // 잘라낸 json 데이터를 출력합니다.

                /*
                * 잘라진 데이터 예시
                {
                    "str":"롯에 씨게 항펀최다.",
                    "errInfo":
                    [
                        {
                            "help":"철자 검사를 해 보니 이 어절은 분석할 수 없으므로 틀린 말로 판단하였습니다.후보 어절은 이 철자검사/교정기에서 띄어쓰기, 붙여 쓰기, 음절대치와 같은 교정방법에 따라 수정한 결과입니다.후 보 어절 중 선택하시거나 오류 어절을 수정하여 주십시오.* 단, 사전에 없는 단어이거나 사용자가 올바르다고 판단한 어절에 대 해서는 통과하세요!!","errorIdx":0,"correctMethod":1,"start":0,"errMsg":"","end":2,"orgStr":"롯에","candWord":"롯데|곳에|옷에|록에"},{"help":"뜻으로 볼 때 바르지 않은 표현입니다.","errorIdx":1,"correctMethod":2,"start":3,"errMsg":"","end":5,"orgStr":"씨게","candWord":"싸게|세게|시게"},{"help":"철자 검사를 해 보니 이 어절은 분석할 수 없으므로 틀린 말로 판단하 였습니다.후보 어절은 이 철자검사/교정기에서 띄어쓰기, 붙여 쓰기, 음절대치와 같은 교정방법에 따라 수정한 결과입니다.후보 어절 중 선택하시거나 오류 어절을 수정하여 주십시오.* 단, 사전에 없는 단어이거나 사용자가 올바르다고 판단한 어절에 대해서는 통과하세요!!",
                            "errorIdx":2,
                            "correctMethod":1,
                            "start":6,
                            "errMsg":"",
                            "end":10,
                            "orgStr":"항펀최다",
                            "candWord":"항 편 최 다"
                        }
                    ],
                    "idx":0
                }
                */

                //auto parse_json = json::json::parse(response.text); // 본격적으로 잘라낸 데이터를 nlohmann json 으로 만듭니다.
                //auto origin = parse_json["errorInfo"]["errorIdx"].get<int>();
                //std::cout << "origin: " << origin << std::endl;
                //auto candWord = parse_json["errorInfo"][0]["candWord"].get<std::string>();
                //
                //std::cout << "origin: " << origin << std::endl;
                //std::cout << "canword: " << candWord << std::endl;
                //
                //origin.erase(std::remove(origin.begin(), origin.end(), ' '), origin.end()); // 스페이스바 삭제
                //candWord.erase(std::remove(candWord.begin(), candWord.end(), ' '), candWord.end()); // 스페이스바 삭제
                //
                //if (origin.compare(candWord.c_str()) == 0) {
                //    std::cout << "대치어가 같습니다" << std::endl;
                //    return true;
                //}
                return false;
            }
        }
        else {
            std::cout << "POST 실패. 오류코드 : " << response.status_code << std::endl;
            return false;
        }
    }
    catch (...) {
        printf("맞춤법 검사 예외발생\n");
    }
}

auto main(void) ->int {
#ifdef TESSERACT_AND_OPENCV_METHOD
    for (int i = 1; i < 5; i++) {
        std::string simple_str = {};
        simple_str = "images\\" + std::to_string(i + 1) + ".jpg";
        auto ocr_texts = read_ocr_words(simple_str, "tessdata");
        if (ocr_texts == "NO_INIT")
            return 0;

        std::cout << "[ " << simple_str << " ]" << Utils::UTF8ToANSI(ocr_texts.c_str()) << std::endl;
        if (check_matchumrule(ocr_texts, true)) {
            SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
            std::cout << "맞춤법이 맞았습니다." << std::endl;
        }
        else {
            SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
            std::cout << "맞춤법이 틀렸습니다." << std::endl;
        }
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 15);
        simple_str.clear();
    }
#endif

    for (int i = 0; i < 11; i++) {
        std::string simple_str;
        simple_str = "images\\" + std::to_string(i + 1) + ".jpg";
        auto ocr_texts = read_ocr_words(simple_str, "tessdata");
        if (ocr_texts == "NO_INIT")
            return 0;

        std::cout << ocr_texts << std::endl;
        if (check_matchumrule(ocr_texts, true)) {
            SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
            std::cout << "맞춤법이 맞았습니다." << std::endl;
        }
        else {
            SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
            std::cout << "맞춤법이 틀렸습니다." << std::endl;
        }
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 15);
    }
    system("pause");
}