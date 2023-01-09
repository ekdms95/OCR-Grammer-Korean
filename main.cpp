#include "Base64.h"
#include "Utils.h"

// �׼���Ʈ OCR �̿��� ���� ��� �߰�
#include <leptonica/allheaders.h>
#include <tesseract/baseapi.h>
#include <cpr/cpr.h> // CURL�� �̿��� ������ HTTP POST GET ���̺귯��
#include <nlohmann/json.hpp> // JSON
#include <opencv2/opencv.hpp> // OpenCV

namespace json = nlohmann; // json ���̺귯���� ���� �������ٰ� �����ϴ� nlohmann ���̺귯���� ����ϰڽ��ϴ�.
//tesseract::TessBaseAPI* api = new tesseract::TessBaseAPI(); // �׼���Ʈ API ��� Ŭ���� ����

// OCR ���
//#define TESSERACT_AND_OPENCV_METHOD // �ּ� Ǯ�� �׼���Ʈ+OpenCV, �ּ��� Ŭ�ι� OCR REST API ���
// �ѱ� OCR�� Ŭ�ιٰ� �е������� �� �н��ϴ�. ( �κ����� )
// 2006����� ���ߵ� ���� ������ ���¼ҽ� OCR ���̺귯�� -> �¼���Ʈ : �ѱ��б� �� ���� ( ���� )


// ����ڰ� �����ؾ��� �͵�
// ***********************************************
#define APIGW_INVOKE_URL "https://5hew6imuyh.apigw.ntruss.com/custom/v1/18892/4703a7e7e66f12451fce6c706327b3cbbcaa35e18b8d084cb3ff39486f84d8ea/general"
#define X_OCR_SECRET_KEY "alBBZnlYeGFxZHJFenNKenFPUU9WV0N6Z2VmdkR2UXI="
// ***********************************************



// OpenCV �Լ� ����� ���� ���������Դϴ�.
int dilation_elem = 0;
int dilation_size = 0;

__declspec(dllexport) auto read_ocr_words(std::string image_path, std::string data_path) -> std::string {
#ifdef TESSERACT_AND_OPENCV_METHOD

    // �׼���Ʈ �ѱ��� �����͸� Init, OEM ������ tesseract::OcrEngineMode::OEM_TESSERACT_LSTM_COMBINED �� �ҽ� �νķ� ����
    // OEM_TESSERACT_LSTM_COMBINED ������ kor2 ������ �ʿ�
    if (api->Init(data_path.c_str(), "kor2", tesseract::OcrEngineMode::OEM_TESSERACT_LSTM_COMBINED))
        return "NO_INIT"; // ���н� ���й��ڿ� ��ȯ

    // kor : �⺻ ���ڵ�����
    // kor2 : �ѱ��� �� �� ������ ������

    //�νķ��� ���̱� ���� opencv�� �̹����� �ٵ���ϴ�.
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
    
    // �̹��� ����׿�
    //cv::imshow("1", newone);
    //cv::waitKey(1);

    return api->GetUTF8Text(); // �׼���Ʈ api����ؼ� �ؽ�Ʈ UTF8�� ����

    // �׼���Ʈ ���̺귯���� ����� �̹��� �ҷ����±���ε� �νķ��� ���Ƽ� �����մϴ�.
    //Pix* image = pixRead(image_path.c_str()); // OCR �̹��� ���� �� �б�
    //api->SetImage(image); // OCR �̹��� ���� 
#else
    // Ŭ�ι� OCR
    unsigned char* test;
    int size;
    Utils::ReadBufferData(image_path, &test, &size);

    json::json request_json;
    request_json["lang"] = "ko"; // �ѱ��� OCR
    request_json["requestId"] = "string";
    request_json["resultType"] = "string";
    request_json["timestamp"] = 0; // 0���� �ص� ������ ����� ��.
    request_json["version"] = "V2"; // V1���ٴ� V2�� �� �νķ��� ���� �����ٰ� Ŭ�ι� docs�� �������־����ϴ�. ������
    request_json["images"][0]["format"] = "jpg"; // �̹��� ���� png, jpg, jpeg, pdf �����Ѵٰ� ����������.
    request_json["images"][0]["name"] = "medium"; // �̰Ŵ� �̹��� �̸��ε� �ƹ����Գ� ������ǰڽ��ϴ�.
    request_json["images"][0]["data"] = base64->base64_decode(base64->base64_encode(test, size)); // �� ��ǻ�� ��ο� �ִ� ���� ���̳ʸ� �����ͷ� �ҷ��ͼ� base64�� ����.
    //request_json["images"][0]["url"] = "https://imgur.com/PYWUoQI.jpg"; Url�� �ִ� �̹��� �̿��Ҷ�

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
            //std::cout << "�νķ��� �����Դϴ�.\n" << std::endl;
        }

        //std::cout << Utils::UTF8ToANSI(rs["images"][0]["fields"][i]["inferText"].get<std::string>().c_str()) << std::endl;
        result_word = result_word + " " + std::string(Utils::UTF8ToANSI(rs["images"][0]["fields"][i]["inferText"].get<std::string>().c_str()));
    }
    //result_word.erase(std::remove(result_word.begin(), result_word.end(), '')); // Ư�� ���� ����
    //std::cout << result_word << std::endl;

    return result_word;
#endif
}

auto check_spacing_rule(std::string src) -> std::string {
    // ���� �˻����
    auto spacing_rule = cpr::Post(
        cpr::Url({ "http://nlp.kookmin.ac.kr/cgi-bin/asp.cgi" }), cpr::Payload{ {"Question", std::string(Utils::UTF8ToANSI(src.c_str())) } }); // �������� �λ���б� ����� �˻�� ����Ʈ�� ����մϴ�.

    std::size_t bp = spacing_rule.text.rfind("<b>���"); // json ���°� �ɶ����� ���ڿ��� �߶��ݴϴ� 1
    std::size_t ep = spacing_rule.text.rfind("</b></td>"); // json ���°� �ɶ����� ���ڿ��� �߶��ݴϴ� 2
    return spacing_rule.text.substr(bp + 9, ep - bp - 9);
}

__declspec(dllexport) auto check_matchumrule(std::string texts, bool debug_strings) -> bool {
    try {
#ifdef TESSERACT_PLUS_OPENCV_METHOD
        std::string spaced_texts = check_spacing_rule(texts);
        std::cout << spaced_texts << std::endl;
#endif

        //std::cout << texts << std::endl;

        // ������˻� ����
        auto response = cpr::Post(
            cpr::Url({ "http://164.125.7.61/speller/results" }), cpr::Parameters{ {"text1", std::string(Utils::ANSItoUTF8(texts.c_str())) } }); // �������� �λ���б� ����� �˻�� ����Ʈ�� ����մϴ�.
        // ����� �˻��Ҷ��� UTF8�� ��ȯ�ؼ� ���ڿ��� ��������մϴ�.

        //std::cout << Utils::UTF8ToANSI(response.text.c_str()) << std::endl;
        if (response.status_code == cpr::status::HTTP_OK) { // HTTP REQUEST 200 - HTTP OK
            if (std::string(Utils::UTF8ToANSI(response.text.c_str())).find("����� �Ѱ�") != std::string::npos) { // ������ ã�� ���ߴٸ�
                std::cout << "������ ã�� ���߽��ϴ�." << std::endl;
                return true; // ������� �ùٸ��� �����մϴ�.
            }
            else {
                std::size_t begin_pos = response.text.rfind("data = ["); // json ���°� �ɶ����� ���ڿ��� �߶��ݴϴ� 1
                std::size_t end_pos = response.text.rfind("}];"); // json ���°� �ɶ����� ���ڿ��� �߶��ݴϴ� 2
                auto temp_filename = response.text.substr(begin_pos, end_pos); // json ���°� �ɶ����� ���ڿ��� �߶��ݴϴ� 3
                auto filename = temp_filename.substr(0, temp_filename.rfind("}];") - 8); // json ���°� �ɶ����� ���ڿ��� �߶��ݴϴ� 4
                //std::cout << Utils::UTF8ToANSI(filename.c_str()) << std::endl; // �߶� json �����͸� ����մϴ�.

                /*
                * �߶��� ������ ����
                {
                    "str":"�Կ� ���� �����ִ�.",
                    "errInfo":
                    [
                        {
                            "help":"ö�� �˻縦 �� ���� �� ������ �м��� �� �����Ƿ� Ʋ�� ���� �Ǵ��Ͽ����ϴ�.�ĺ� ������ �� ö�ڰ˻�/�����⿡�� ����, �ٿ� ����, ������ġ�� ���� ��������� ���� ������ ����Դϴ�.�� �� ���� �� �����Ͻðų� ���� ������ �����Ͽ� �ֽʽÿ�.* ��, ������ ���� �ܾ��̰ų� ����ڰ� �ùٸ��ٰ� �Ǵ��� ������ �� �ؼ��� ����ϼ���!!","errorIdx":0,"correctMethod":1,"start":0,"errMsg":"","end":2,"orgStr":"�Կ�","candWord":"�Ե�|����|�ʿ�|�Ͽ�"},{"help":"������ �� �� �ٸ��� ���� ǥ���Դϴ�.","errorIdx":1,"correctMethod":2,"start":3,"errMsg":"","end":5,"orgStr":"����","candWord":"�ΰ�|����|�ð�"},{"help":"ö�� �˻縦 �� ���� �� ������ �м��� �� �����Ƿ� Ʋ�� ���� �Ǵ��� �����ϴ�.�ĺ� ������ �� ö�ڰ˻�/�����⿡�� ����, �ٿ� ����, ������ġ�� ���� ��������� ���� ������ ����Դϴ�.�ĺ� ���� �� �����Ͻðų� ���� ������ �����Ͽ� �ֽʽÿ�.* ��, ������ ���� �ܾ��̰ų� ����ڰ� �ùٸ��ٰ� �Ǵ��� ������ ���ؼ��� ����ϼ���!!",
                            "errorIdx":2,
                            "correctMethod":1,
                            "start":6,
                            "errMsg":"",
                            "end":10,
                            "orgStr":"�����ִ�",
                            "candWord":"�� �� �� ��"
                        }
                    ],
                    "idx":0
                }
                */

                //auto parse_json = json::json::parse(response.text); // ���������� �߶� �����͸� nlohmann json ���� ����ϴ�.
                //auto origin = parse_json["errorInfo"]["errorIdx"].get<int>();
                //std::cout << "origin: " << origin << std::endl;
                //auto candWord = parse_json["errorInfo"][0]["candWord"].get<std::string>();
                //
                //std::cout << "origin: " << origin << std::endl;
                //std::cout << "canword: " << candWord << std::endl;
                //
                //origin.erase(std::remove(origin.begin(), origin.end(), ' '), origin.end()); // �����̽��� ����
                //candWord.erase(std::remove(candWord.begin(), candWord.end(), ' '), candWord.end()); // �����̽��� ����
                //
                //if (origin.compare(candWord.c_str()) == 0) {
                //    std::cout << "��ġ� �����ϴ�" << std::endl;
                //    return true;
                //}
                return false;
            }
        }
        else {
            std::cout << "POST ����. �����ڵ� : " << response.status_code << std::endl;
            return false;
        }
    }
    catch (...) {
        printf("����� �˻� ���ܹ߻�\n");
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
            std::cout << "������� �¾ҽ��ϴ�." << std::endl;
        }
        else {
            SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
            std::cout << "������� Ʋ�Ƚ��ϴ�." << std::endl;
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
            std::cout << "������� �¾ҽ��ϴ�." << std::endl;
        }
        else {
            SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
            std::cout << "������� Ʋ�Ƚ��ϴ�." << std::endl;
        }
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 15);
    }
    system("pause");
}