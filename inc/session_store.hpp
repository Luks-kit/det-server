#include <map>
#include <string>

class SessionStore {
public:
    static void save(const std::string& sid, const std::string& user) {
        sessions[sid] = user;
    }

    static std::string get(const std::string& sid) {
        if (sessions.count(sid))
            return sessions[sid];
        return "";
    }

private:
    static std::map<std::string, std::string> sessions;
};
