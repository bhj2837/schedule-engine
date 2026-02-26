#include "crow_all.h"
#include "scheduler.h"
#include <fstream>
#include <sstream>

// Reads a file into a string. Falls back to inline message if not found.
static std::string readFile(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) return "";
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

int main() {
    crow::SimpleApp app;

    // ── Dashboard UI ───────────────────────────────────────────────
    CROW_ROUTE(app, "/")([]() {
        std::string html = readFile("dashboard.html");
        if (html.empty()) {
            // Fallback if file not found next to exe
            html = readFile("../dashboard.html");
        }
        if (html.empty()) {
            return crow::response(200,
                "<h2>Schedule Engine is running on port 8080</h2>"
                "<p>Place <b>dashboard.html</b> next to the executable to load the UI.</p>");
        }
        crow::response res(200, html);
        res.set_header("Content-Type", "text/html; charset=utf-8");
        return res;
    });

    // ── GET /tasks ─────────────────────────────────────────────────
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
            item["deadline"]     = t.deadline;
            list.push_back(std::move(item));
        }
        result["tasks"] = std::move(list);

        crow::response res(200, result.dump());
        res.set_header("Content-Type", "application/json");
        return res;
    });

    // ── POST /schedule ─────────────────────────────────────────────
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
            return err;
        }

        crow::response res(200, out.dump());
        res.set_header("Content-Type", "application/json");
        return res;
    });

    app.port(8080).multithreaded().run();
    return 0;
}
