#include "scheduler.h"
#include <sstream>
#include <iomanip>
#include <ctime>
#include <regex>

// ───────────────────────────────────────────
// 내부 유틸
// ───────────────────────────────────────────
static bool isDeadlineFormat(const std::string& s) {
    // "YYYY-MM-DDTHH:MM:SS" 형식 검사
    static const std::regex pattern(
        R"(\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2})");
    return std::regex_match(s, pattern);
}

static long long parseDeadline(const std::string& s) {
    std::tm tm = {};
    std::istringstream ss(s);
    ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
    tm.tm_isdst = -1;
    return (long long)mktime(&tm);
}

// ───────────────────────────────────────────
// parseTasks: 유효성 검사 포함
// ───────────────────────────────────────────
bool Scheduler::parseTasks(const crow::json::rvalue& body,
                            std::vector<Task>&        out,
                            std::string&              errMsg)
{
    if (!body.has("tasks")) {
        errMsg = "Missing required field: tasks";
        return false;
    }

    int index = 0;
    for (auto& item : body["tasks"]) {
        std::string prefix = "tasks[" + std::to_string(index) + "]: ";

        // ── 필수 필드 존재 확인
        for (const char* field : {"id", "name", "duration", "priority", "deadline"}) {
            if (!item.has(field)) {
                errMsg = prefix + "Missing required field: " + field;
                return false;
            }
        }

        // ── 타입 검사
        try {
            Task t;
            t.id           = item["id"].i();
            t.title        = item["name"].s();
            t.duration_min = item["duration"].i();
            t.priority     = item["priority"].i();

            // deadline 형식 검사
            std::string dl = item["deadline"].s();
            if (!isDeadlineFormat(dl)) {
                errMsg = prefix + "Invalid deadline format. Expected: YYYY-MM-DDTHH:MM:SS";
                return false;
            }
            t.deadline = parseDeadline(dl);
            if (t.deadline == -1) {
                errMsg = prefix + "Invalid deadline value";
                return false;
            }

            // duration 양수 검사
            if (t.duration_min <= 0) {
                errMsg = prefix + "duration must be a positive integer";
                return false;
            }

            // priority 범위 검사 (1~10)
            if (t.priority < 1 || t.priority > 10) {
                errMsg = prefix + "priority must be between 1 and 10";
                return false;
            }

            out.push_back(t);
        }
        catch (const std::exception&) {
            errMsg = prefix + "Type error in one or more fields"
                     " (id/duration/priority must be integers, name/deadline must be strings)";
            return false;
        }

        ++index;
    }

    if (out.empty()) {
        errMsg = "tasks array is empty";
        return false;
    }

    return true;
}

// ───────────────────────────────────────────
// schedule: EDF + 우선순위 가중치
// ───────────────────────────────────────────
std::vector<ScheduledBlock> Scheduler::schedule(std::vector<Task>& tasks,
                                                  long long available_start)
{
    std::priority_queue<Task, std::vector<Task>, std::greater<Task>> pq;
    for (auto& t : tasks) pq.push(t);

    std::vector<ScheduledBlock> result;
    long long current_time = available_start;

    while (!pq.empty()) {
        Task t = pq.top(); pq.pop();
        ScheduledBlock block;
        block.task       = t;
        block.start_time = current_time;
        block.end_time   = current_time + (t.duration_min * 60LL);
        result.push_back(block);
        current_time = block.end_time;
    }
    return result;
}

// ───────────────────────────────────────────
// optimize: 파싱 → 스케줄 → 저장 → 응답 조립
// ───────────────────────────────────────────
bool Scheduler::optimize(const crow::json::rvalue& body,
                          crow::json::wvalue&        out,
                          crow::response&            err)
{
    // 1) 파싱 + 검증
    std::vector<Task> tasks;
    std::string errMsg;
    if (!parseTasks(body, tasks, errMsg)) {
        err = errorResponse(400, errMsg);
        return false;
    }

    // 2) available_start
    long long start_time = body.has("available_start")
        ? body["available_start"].i()
        : (long long)time(nullptr);

    // 3) 스케줄링
    auto scheduled = schedule(tasks, start_time);

    // 4) TaskStore에 upsert (POST /schedule 호출 시 자동 저장)
    auto& store = TaskStore::instance();
    for (auto& t : tasks) store.upsert(t);

    // 5) 응답 조립
    out["status"] = "ok";
    out["count"]  = (int)scheduled.size();

    std::vector<crow::json::wvalue> blocks;
    blocks.reserve(scheduled.size());
    for (auto& b : scheduled) {
        crow::json::wvalue block;
        block["id"]         = b.task.id;
        block["title"]      = b.task.title;
        block["start_time"] = b.start_time;
        block["end_time"]   = b.end_time;
        block["priority"]   = b.task.priority;
        block["duration_min"] = b.task.duration_min;
        blocks.push_back(std::move(block));
    }
    out["schedule"] = std::move(blocks);
    return true;
}