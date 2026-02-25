#include "crow_all.h"
#include "scheduler.h"

int main() {
    crow::SimpleApp app;

    // ── GET / ────────────────────────────────
    CROW_ROUTE(app, "/")([]() {
        return "Schedule Engine is running!";
    });

    // ── GET /tasks ───────────────────────────
    // 저장된 태스크 목록 반환
    CROW_ROUTE(app, "/tasks")([]() {
        auto tasks = TaskStore::instance().getAll();

        crow::json::wvalue result;
        result["status"] = "ok";
        result["count"]  = (int)tasks.size();

        std::vector<crow::json::wvalue> list;
        list.reserve(tasks.size());
        for (auto& t : tasks) {
            crow::json::wvalue item;
            item["id"]           = t.id;
            item["name"]         = t.title;
            item["duration_min"] = t.duration_min;
            item["priority"]     = t.priority;
            item["deadline"]     = t.deadline;   // Unix timestamp
            list.push_back(std::move(item));
        }
        result["tasks"] = std::move(list);

        return crow::response(result.dump());
    });

    // ── POST /schedule ───────────────────────
    // 스케줄 최적화 + TaskStore 자동 저장
    CROW_ROUTE(app, "/schedule").methods("POST"_method)
    ([](const crow::request& req) {
        auto body = crow::json::load(req.body);
        if (!body) {
            return errorResponse(400, "Invalid JSON");
        }

        Scheduler scheduler;
        crow::json::wvalue out;
        crow::response     err;

        if (!scheduler.optimize(body, out, err)) {
            return err;   // 400 + 에러 메시지
        }

        crow::response res(200, out.dump());
        res.set_header("Content-Type", "application/json");
        return res;
    });

    app.port(8080).multithreaded().run();
    return 0;
}