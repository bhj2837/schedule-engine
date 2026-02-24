#include "crow_all.h"
#include "scheduler.h"

int main() {
    crow::SimpleApp app;

    // 서버 동작 확인용
    CROW_ROUTE(app, "/")([]() {
        return "Schedule Engine is running!";
    });

    // 태스크 목록 조회
    CROW_ROUTE(app, "/tasks")([]() {
        crow::json::wvalue result;
        result["status"] = "ok";
        result["message"] = "task list endpoint";
        return result;
    });

    // 스케줄 최적화 실행
    CROW_ROUTE(app, "/schedule").methods("POST"_method)
    ([](const crow::request& req) {
        auto body = crow::json::load(req.body);
        if (!body) {
            return crow::response(400, "Invalid JSON");
        }

        Scheduler scheduler;
        auto result = scheduler.optimize(body);
        return crow::response(result.dump());
    });

    app.port(8080).multithreaded().run();
    return 0;
}