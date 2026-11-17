// dialog.h — minimal dialogue runtime, v0.8
//
// Runtime is default constructible; call load_scene()/load_scene_files() later.
//
// Changelog from v0.7:
//  - Jump node: {"type":"jump","target":"graph.local" | "local"}.
//    Silent; hands control to another node, possibly in another graph
//    within the same stage.
//  - Structured errors: LoadError{node_path, field, message} instead of
//    strings. to_string() preserves the old format for CLI use.
//
// Logging (diagnostic only, no API or behaviour change). Output goes to:
//  - Android:    logcat via __android_log_print, tag "dialog" (needs -llog;
//                define DIALOG_LOG_NO_ANDROID to use stderr instead, which
//                Android discards).
//  - Web:        browser console via emscripten_log (Emscripten builds).
//  - Desktop:    stderr (Windows/macOS/Linux/iOS). GUI-subsystem Windows apps
//                have no stderr: use DIALOG_LOG_SINK (e.g. OutputDebugString).
// Knobs (define before including this header):
//  - DIALOG_LOG_LEVEL: 0=error 1=warn 2=info 3=debug/trace, -1=off.
//                Default 3 when NDEBUG is not defined, otherwise 2.
//  - DIALOG_LOG_TAG:   Android logcat tag (default "dialog").
//  - DIALOG_LOG_SINK(level_int, std::string): route logs anywhere, all platforms.
//  - DIALOG_NO_EXCEPTIONS: force-disable the try/catch based diagnostics.
//                Auto-detected from the compiler, so -fno-exceptions builds
//                (common on Android/Web) compile fine.

#pragma once
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <exception>
#include <fstream>
#include <functional>
#include <limits>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#include "nlohmann/json.hpp"

// ---- platform detection (logging + diagnostics only)
#if defined(__EMSCRIPTEN__)
#  define DIALOG_PLATFORM_NAME "web"
#elif defined(__ANDROID__)
#  define DIALOG_PLATFORM_NAME "android"
#elif defined(_WIN32)
#  define DIALOG_PLATFORM_NAME "windows"
#elif defined(__APPLE__)
#  define DIALOG_PLATFORM_NAME "apple"
#elif defined(__linux__)
#  define DIALOG_PLATFORM_NAME "linux"
#else
#  define DIALOG_PLATFORM_NAME "unknown"
#endif

#if !defined(DIALOG_LOG_SINK)
#  if defined(__EMSCRIPTEN__)
#    include <emscripten/emscripten.h>
#  elif defined(__ANDROID__) && !defined(DIALOG_LOG_NO_ANDROID)
#    include <android/log.h>
#  endif
#endif

// try/catch is a compile error under -fno-exceptions; only use it when the
// compiler says exceptions are on.
#if !defined(DIALOG_NO_EXCEPTIONS) && \
    (defined(__cpp_exceptions) || defined(__EXCEPTIONS) || defined(_CPPUNWIND))
#  define DIALOG_HAS_EXCEPTIONS 1
#endif

namespace dialog {

// ---------------------------------------------------------------- public

enum class FrameKind { Line, Choice, Ended, Error };

struct Frame {
    FrameKind                kind = FrameKind::Ended;
    std::string              speaker;
    std::string              speaker_name;
    std::string              text;
    std::vector<std::string> choices;
    std::string              error;
};

struct LoadError {
    std::string node_path;   // "stage.graph.local", or "" if not node-specific
    std::string field;       // "next", "cond", "target", ..., or "" if not field-specific
    std::string message;
};

inline std::string to_string(const LoadError& e) {
    std::string s;
    if (!e.node_path.empty()) s += e.node_path + ": ";
    if (!e.field.empty())     s += e.field + ": ";
    s += e.message;
    return s;
}

// ---------------------------------------------------------------- detail

namespace detail {

// ------------------------------------------------------------- logging

#ifndef DIALOG_LOG_TAG
#define DIALOG_LOG_TAG "dialog"
#endif
#ifndef DIALOG_LOG_LEVEL
#  ifdef NDEBUG
#    define DIALOG_LOG_LEVEL 2
#  else
#    define DIALOG_LOG_LEVEL 3
#  endif
#endif

enum class Log { Error = 0, Warn = 1, Info = 2, Debug = 3 };

inline void log_write(Log lvl, const std::string& msg) {
#if defined(DIALOG_LOG_SINK)
    DIALOG_LOG_SINK(static_cast<int>(lvl), msg);
#elif defined(__EMSCRIPTEN__)
    const int flags = EM_LOG_CONSOLE
                    | (lvl == Log::Error ? EM_LOG_ERROR
                     : lvl == Log::Warn  ? EM_LOG_WARN
                     : lvl == Log::Info  ? EM_LOG_INFO
                                         : EM_LOG_DEBUG);
    emscripten_log(flags, "[dialog] %s", msg.c_str());
#elif defined(__ANDROID__) && !defined(DIALOG_LOG_NO_ANDROID)
    const int prio = lvl == Log::Error ? ANDROID_LOG_ERROR
                   : lvl == Log::Warn  ? ANDROID_LOG_WARN
                   : lvl == Log::Info  ? ANDROID_LOG_INFO
                                       : ANDROID_LOG_DEBUG;
    __android_log_print(prio, DIALOG_LOG_TAG, "%s", msg.c_str());
#else
    const char* tag = lvl == Log::Error ? "E" : lvl == Log::Warn ? "W"
                    : lvl == Log::Info  ? "I" : "D";
    std::fprintf(stderr, "[dialog][%s] %s\n", tag, msg.c_str());
    std::fflush(stderr);
#endif
}

// Usage: DIALOG_LOG(Warn, "x=" << x);   (stream-style, evaluated lazily)
#define DIALOG_LOG(lvl, expr)                                                   \
    do {                                                                        \
        if (static_cast<int>(::dialog::detail::Log::lvl) <= DIALOG_LOG_LEVEL) { \
            std::ostringstream dialog_log_os_;                                  \
            dialog_log_os_ << expr;                                             \
            ::dialog::detail::log_write(::dialog::detail::Log::lvl,             \
                                        dialog_log_os_.str());                  \
        }                                                                       \
    } while (0)

#if defined(__ANDROID__)
#  define DIALOG_FILE_HINT \
     "Android: files inside the APK/assets are NOT reachable via std::ifstream - " \
     "read them with AAssetManager (or copy to internal storage) and call load_scene()"
#elif defined(__EMSCRIPTEN__)
#  define DIALOG_FILE_HINT \
     "Web: the file must exist in Emscripten's virtual FS - use --preload-file / " \
     "--embed-file, or fetch() it and call load_scene() with the text"
#elif defined(_WIN32)
#  define DIALOG_FILE_HINT \
     "Windows: check the working directory; narrow paths use the ANSI code page, " \
     "so non-ASCII paths may fail"
#else
#  define DIALOG_FILE_HINT "check the path and the process working directory"
#endif

inline const char* kind_name(FrameKind k) {
    switch (k) {
        case FrameKind::Line:   return "Line";
        case FrameKind::Choice: return "Choice";
        case FrameKind::Ended:  return "Ended";
        case FrameKind::Error:  return "Error";
    }
    return "?";
}

// Pushes a warning to the caller's vector AND logs it.
inline void warn(std::vector<std::string>& w, std::string msg) {
    DIALOG_LOG(Warn, msg);
    w.push_back(std::move(msg));
}

// Why did nlohmann refuse this input? Re-parses with exceptions to get the
// message (includes byte offset). Without exceptions we can only say so.
inline std::string json_parse_failure_reason(const std::string& text) {
    if (text.empty())
        return "input is EMPTY (file missing/unreadable - see the 'cannot open' "
               "log; platform=" DIALOG_PLATFORM_NAME ")";
#ifdef DIALOG_HAS_EXCEPTIONS
    try { (void)nlohmann::json::parse(text); }
    catch (const std::exception& ex) { return ex.what(); }
    return "unknown";
#else
    return "unknown (exceptions disabled in this build, no detail available)";
#endif
}

inline std::string clip(const std::string& s, size_t n = 48) {
    return s.size() <= n ? s : s.substr(0, n) + "...";
}

using NodeId = uint32_t;
using ExprId = uint32_t;
constexpr NodeId kInvalid = std::numeric_limits<NodeId>::max();
constexpr ExprId kNoExpr  = std::numeric_limits<ExprId>::max();

enum class VarType { Bool, Int };
enum class SetOp   { Assign, Add, Sub, Not };
enum class NodeType { Line, Choice, If, Set, Jump, End };

inline const char* node_type_name(NodeType t) {
    switch (t) {
        case NodeType::Line:   return "Line";
        case NodeType::Choice: return "Choice";
        case NodeType::If:     return "If";
        case NodeType::Set:    return "Set";
        case NodeType::Jump:   return "Jump";
        case NodeType::End:    return "End";
    }
    return "?";
}

inline void err(std::vector<LoadError>& e, std::string path,
                std::string field, std::string msg)
{
    LoadError le{std::move(path), std::move(field), std::move(msg)};
    DIALOG_LOG(Error, "load error: " << to_string(le));
    e.push_back(std::move(le));
}

struct Expr {
    enum class Kind {
        LitBool, LitInt, VarBool, VarInt,
        And, Or, Not, Eq, Ne, Lt, Le, Gt, Ge
    };
    Kind      kind = Kind::LitBool;
    bool      bval = false;
    int64_t   ival = 0;
    std::string var;
    std::vector<ExprId> kids;
};

inline VarType expr_result_type(Expr::Kind k) {
    switch (k) {
        case Expr::Kind::LitInt:
        case Expr::Kind::VarInt: return VarType::Int;
        default:                 return VarType::Bool;
    }
}

struct Choice {
    std::string text;
    NodeId      target = kInvalid;
    ExprId      cond   = kNoExpr;
};

struct Node {
    NodeType    type = NodeType::End;
    std::string speaker;
    std::string text;
    uint32_t    choice_begin = 0;
    uint32_t    choice_count = 0;
    ExprId      cond  = kNoExpr;
    NodeId      then_ = kInvalid;
    NodeId      else_ = kInvalid;
    std::string set_var;
    SetOp       set_op = SetOp::Assign;
    bool        set_b  = false;
    int64_t     set_i  = 0;
    NodeId      next   = kInvalid;   // Line/Set/Jump
};

struct Placement {
    std::string id;
    std::string character;
    std::string entry;
};

struct Stage {
    std::string                            id;
    std::vector<Placement>                 placements;
    std::unordered_map<std::string, NodeId> graph_entries;
    // "graph.local" -> NodeId, filled during graph compilation.
    std::unordered_map<std::string, NodeId> node_lookup;
};

struct PendingJump {
    NodeId      source;   // the jump node itself
    std::string target;   // "graph.local"
    std::string where;    // for error reporting
};

struct SceneData {
    std::string id;
    std::string entry_stage;

    std::unordered_map<std::string, Stage>       stages;
    std::unordered_map<std::string, std::string> characters;

    std::vector<Node>   nodes;
    std::vector<Choice> choices;
    std::vector<Expr>   exprs;

    std::unordered_map<std::string, bool>    bool_defaults;
    std::unordered_map<std::string, int64_t> int_defaults;
    std::unordered_map<std::string, VarType> var_types;

    std::vector<std::string>                 node_paths;
    std::unordered_map<std::string, NodeId>  path_to_node;
};

// "stage.graph.node" or a marker for kInvalid / out-of-range ids.
inline std::string node_label(const SceneData& s, NodeId id) {
    if (id == kInvalid)           return "<none>";
    if (id >= s.node_paths.size()) return "<bad id " + std::to_string(id) + ">";
    return s.node_paths[id];
}

// ---------------------------------------------------------- expr compiler

inline ExprId compile_expr(SceneData& s, const nlohmann::json& je,
                           const std::string& where,
                           std::vector<LoadError>& errors)
{
    if (!je.is_object()) { err(errors, where, "cond", "not an object"); return kNoExpr; }

    if (je.contains("var")) {
        if (!je["var"].is_string()) {
            err(errors, where, "cond", "var not a string");
            return kNoExpr;
        }
        const std::string name = je["var"].get<std::string>();
        auto it = s.var_types.find(name);
        if (it == s.var_types.end()) {
            err(errors, where, "cond", "undeclared variable '" + name + "'");
            return kNoExpr;
        }
        Expr e;
        e.kind = (it->second == VarType::Bool) ? Expr::Kind::VarBool : Expr::Kind::VarInt;
        e.var  = name;
        s.exprs.push_back(std::move(e));
        return static_cast<ExprId>(s.exprs.size() - 1);
    }

    if (je.contains("lit")) {
        const auto& jv = je["lit"];
        Expr e;
        if (jv.is_boolean())             { e.kind = Expr::Kind::LitBool; e.bval = jv.get<bool>(); }
        else if (jv.is_number_integer()) { e.kind = Expr::Kind::LitInt;  e.ival = jv.get<int64_t>(); }
        else { err(errors, where, "cond", "unsupported literal"); return kNoExpr; }
        s.exprs.push_back(std::move(e));
        return static_cast<ExprId>(s.exprs.size() - 1);
    }

    const std::string op = je.value("op", "");
    if (op.empty() || !je.contains("kids") || !je["kids"].is_array()) {
        err(errors, where, "cond", "malformed op node");
        return kNoExpr;
    }

    Expr e;
    std::vector<VarType> kt;
    for (const auto& jk : je["kids"]) {
        ExprId ki = compile_expr(s, jk, where, errors);
        if (ki == kNoExpr) return kNoExpr;
        e.kids.push_back(ki);
        kt.push_back(expr_result_type(s.exprs[ki].kind));
    }

    auto arity = [&](size_t n) {
        if (e.kids.size() != n) {
            err(errors, where, "cond", "'" + op + "' wrong arity");
            return false;
        }
        return true;
    };
    auto all_bool = [&]() {
        for (auto t : kt) if (t != VarType::Bool) {
            err(errors, where, "cond", "'" + op + "' needs bool kids");
            return false;
        }
        return true;
    };
    auto all_int = [&]() {
        for (auto t : kt) if (t != VarType::Int) {
            err(errors, where, "cond", "'" + op + "' needs int kids");
            return false;
        }
        return true;
    };

    if (op == "and" || op == "or") {
        if (e.kids.size() < 2 || !all_bool()) return kNoExpr;
        e.kind = (op == "and") ? Expr::Kind::And : Expr::Kind::Or;
    } else if (op == "not") {
        if (!arity(1) || !all_bool()) return kNoExpr;
        e.kind = Expr::Kind::Not;
    } else if (op == "eq" || op == "ne") {
        if (!arity(2)) return kNoExpr;
        if (kt[0] != kt[1]) {
            err(errors, where, "cond", "'" + op + "' operands must match");
            return kNoExpr;
        }
        e.kind = (op == "eq") ? Expr::Kind::Eq : Expr::Kind::Ne;
    } else if (op == "lt" || op == "le" || op == "gt" || op == "ge") {
        if (!arity(2) || !all_int()) return kNoExpr;
        e.kind = (op == "lt") ? Expr::Kind::Lt
               : (op == "le") ? Expr::Kind::Le
               : (op == "gt") ? Expr::Kind::Gt
                              : Expr::Kind::Ge;
    } else {
        err(errors, where, "cond", "unknown op '" + op + "'");
        return kNoExpr;
    }

    s.exprs.push_back(std::move(e));
    return static_cast<ExprId>(s.exprs.size() - 1);
}

// ----------------------------------------------------------- set compiler

inline void compile_set(SceneData& s, Node& n, const nlohmann::json& jn,
                        const std::string& where, std::vector<LoadError>& errors)
{
    n.type = NodeType::Set;

    const std::string name = jn.value("var", "");
    auto vt = s.var_types.find(name);
    if (name.empty() || vt == s.var_types.end()) {
        err(errors, where, "var", "unknown variable '" + name + "'");
        return;
    }
    n.set_var = name;

    const std::string op = jn.value("op", "");
    if      (op == "set") n.set_op = SetOp::Assign;
    else if (op == "add") n.set_op = SetOp::Add;
    else if (op == "sub") n.set_op = SetOp::Sub;
    else if (op == "not") n.set_op = SetOp::Not;
    else { err(errors, where, "op", "unknown set op '" + op + "'"); return; }

    if (n.set_op == SetOp::Not) {
        if (vt->second != VarType::Bool)
            err(errors, where, "op", "'not' on non-bool");
        return;
    }
    if (!jn.contains("value")) {
        err(errors, where, "value", "missing");
        return;
    }

    const auto& jv = jn["value"];
    if (vt->second == VarType::Bool) {
        if (n.set_op != SetOp::Assign)
            err(errors, where, "op", "not valid for bool");
        else if (!jv.is_boolean())
            err(errors, where, "value", "bool required");
        else
            n.set_b = jv.get<bool>();
    } else {
        if (!jv.is_number_integer())
            err(errors, where, "value", "int required");
        else
            n.set_i = jv.get<int64_t>();
    }
}

// --------------------------------------------------------- graph compiler

inline void compile_graph(SceneData& s, Stage& stage,
                          const std::string& graph_name,
                          const nlohmann::json& jg,
                          const std::function<std::string(const std::string&)>& resolve,
                          std::vector<PendingJump>& pending,
                          std::vector<LoadError>& errors)
{
    if (!jg.contains("nodes") || !jg["nodes"].is_object()) {
        err(errors, stage.id + "." + graph_name, "nodes", "missing");
        return;
    }

    DIALOG_LOG(Debug, "compile graph '" << stage.id << "." << graph_name << "': "
                      << jg["nodes"].size() << " node(s), start='"
                      << jg.value("start", "") << "'");

    std::unordered_map<std::string, NodeId> local;
    for (auto it = jg["nodes"].begin(); it != jg["nodes"].end(); ++it) {
        const std::string& lid = it.key();
        NodeId id = static_cast<NodeId>(s.nodes.size());
        s.nodes.emplace_back();

        if (lid.find('.') != std::string::npos)
            err(errors, stage.id + "." + graph_name + "." + lid,
                "id", "must not contain '.'");

        const std::string full_path = stage.id + "." + graph_name + "." + lid;
        s.node_paths.push_back(full_path);
        s.path_to_node[full_path] = id;

        // Local graph lookup and stage-wide lookup for Jump.
        stage.node_lookup[graph_name + "." + lid] = id;
        local[lid] = id;
    }

    auto lookup = [&](const std::string& lid) -> NodeId {
        auto f = local.find(lid);
        return f == local.end() ? kInvalid : f->second;
    };
    auto check = [&](const std::string& target, const std::string& where,
                     const std::string& field) -> NodeId {
        NodeId id = lookup(target);
        if (id == kInvalid)
            err(errors, where, field, "bad ref '" + target + "'");
        return id;
    };

    for (auto it = jg["nodes"].begin(); it != jg["nodes"].end(); ++it) {
        const std::string& lid = it.key();
        const auto& jn = it.value();
        Node& n = s.nodes[local[lid]];

        const std::string type  = jn.value("type", "");
        const std::string where = stage.id + "." + graph_name + "." + lid;

        if (type == "line") {
            n.type    = NodeType::Line;
            n.speaker = jn.value("speaker", "");
            n.text    = resolve(jn.value("text", ""));
            const std::string next = jn.value("next", "");
            if (!next.empty()) n.next = check(next, where, "next");
            else DIALOG_LOG(Debug, where << ": line has no 'next' -> conversation "
                                   "ends after it (typo in the key?)");
            if (n.text.empty())
                DIALOG_LOG(Warn, where << ": line has EMPTY text "
                                  "(missing/empty 'text' key?) - UI may show nothing");
        } else if (type == "choice") {
            n.type = NodeType::Choice;
            n.choice_begin = static_cast<uint32_t>(s.choices.size());
            if (jn.contains("choices") && jn["choices"].is_array()) {
                for (const auto& jc : jn["choices"]) {
                    Choice c;
                    c.text   = resolve(jc.value("text", ""));
                    c.target = check(jc.value("next", ""), where, "choices.next");
                    if (jc.contains("if")) {
                        c.cond = compile_expr(s, jc["if"], where, errors);
                    }
                    s.choices.push_back(std::move(c));
                }
            }
            n.choice_count = static_cast<uint32_t>(s.choices.size()) - n.choice_begin;
            if (n.choice_count == 0)
                DIALOG_LOG(Warn, where << ": choice node has no choices "
                                  "(missing/invalid 'choices' array) - will yield "
                                  "Error 'no visible choices' at runtime");
        } else if (type == "if") {
            n.type = NodeType::If;
            if (jn.contains("cond"))
                n.cond = compile_expr(s, jn["cond"], where, errors);
            else
                err(errors, where, "cond", "missing");
            n.then_ = check(jn.value("then", ""), where, "then");
            n.else_ = check(jn.value("else", ""), where, "else");
        } else if (type == "set") {
            compile_set(s, n, jn, where, errors);
            n.next = check(jn.value("next", ""), where, "next");
        } else if (type == "jump") {
            n.type = NodeType::Jump;
            const std::string target = jn.value("target", "");
            if (target.empty()) {
                err(errors, where, "target", "missing");
            } else if (target.find('.') == std::string::npos) {
                // Same-graph target: resolve now.
                n.next = check(target, where, "target");
            } else {
                // Cross-graph target within this stage: defer.
                pending.push_back({local[lid], target, where});
            }
        } else if (type == "end") {
            n.type = NodeType::End;
        } else {
            err(errors, where, "type", "unknown '" + type + "'");
        }
    }

    const std::string start = jg.value("start", "");
    NodeId start_id = lookup(start);
    if (start_id == kInvalid)
        err(errors, stage.id + "." + graph_name, "start", "bad ref '" + start + "'");
    else
        stage.graph_entries[graph_name] = start_id;
    DIALOG_LOG(Debug, "graph '" << stage.id << "." << graph_name << "' start -> "
                      << (start_id == kInvalid ? "UNRESOLVED (graph has no entry)"
                                               : s.node_paths[start_id]));
}

// --------------------------------------------------------- scene compiler

inline void compile_scene(SceneData& s, const std::string& json,
                          const std::unordered_map<std::string, std::string>& strings,
                          std::vector<LoadError>& errors)
{
    auto j = nlohmann::json::parse(json, nullptr, false);
    if (j.is_discarded()) {
        const std::string why = json_parse_failure_reason(json);
        DIALOG_LOG(Error, "scene JSON parse failed (" << json.size() << " bytes): " << why);
        err(errors, "", "", "scene: invalid JSON");
        return;
    }

    auto resolve = [&](const std::string& key) -> std::string {
        if (key.empty()) return {};
        auto it = strings.find(key);
        if (it != strings.end()) return it->second;
        err(errors, "", "text", "missing string key '" + key + "'");
        return key;
    };

    if (j.contains("variables") && j["variables"].is_object()) {
        for (auto it = j["variables"].begin(); it != j["variables"].end(); ++it) {
            const std::string type = it.value().value("type", "");
            if (type == "bool") {
                s.var_types[it.key()]     = VarType::Bool;
                s.bool_defaults[it.key()] = it.value().value("default", false);
            } else if (type == "int") {
                s.var_types[it.key()]    = VarType::Int;
                s.int_defaults[it.key()] = it.value().value("default", int64_t{0});
            } else {
                err(errors, "variables." + it.key(), "type", "unknown '" + type + "'");
            }
        }
    }

    if (j.contains("characters") && j["characters"].is_object()) {
        for (auto it = j["characters"].begin(); it != j["characters"].end(); ++it) {
            auto nit = it.value().find("name");
            s.characters[it.key()] = (nit == it.value().end())
                ? it.key() : resolve(nit->get<std::string>());
        }
    }

    if (!j.contains("scene") || !j["scene"].is_object()) {
        err(errors, "", "", "scene: missing");
        return;
    }
    const auto& js = j["scene"];
    s.id          = js.value("id", "");
    s.entry_stage = js.value("entry_stage", "");

    if (!js.contains("stages") || !js["stages"].is_object()) {
        err(errors, "", "", "scene: no stages");
        return;
    }

    for (auto it = js["stages"].begin(); it != js["stages"].end(); ++it) {
        const std::string stage_id = it.key();
        const auto& jst = it.value();

        Stage stage;
        stage.id = stage_id;

        if (jst.contains("placements") && jst["placements"].is_array()) {
            for (const auto& jp : jst["placements"]) {
                Placement p;
                p.id        = jp.value("id", "");
                p.character = jp.value("character", "");
                p.entry     = jp.value("entry", "");
                if (p.id.empty() || p.character.empty() || p.entry.empty())
                    err(errors, stage_id + ".placements", "placement", "incomplete");
                else
                    stage.placements.push_back(std::move(p));
            }
        }

        std::vector<PendingJump> pending;

        if (jst.contains("graphs") && jst["graphs"].is_object()) {
            for (auto git = jst["graphs"].begin(); git != jst["graphs"].end(); ++git)
                compile_graph(s, stage, git.key(), git.value(),
                              resolve, pending, errors);
        }

        // Resolve deferred cross-graph jumps now that all graphs are known.
        for (auto& pj : pending) {
            auto nit = stage.node_lookup.find(pj.target);
            if (nit == stage.node_lookup.end()) {
                err(errors, pj.where, "target", "bad ref '" + pj.target + "'");
            } else {
                s.nodes[pj.source].next = nit->second;
            }
        }

        for (const auto& p : stage.placements)
            if (!stage.graph_entries.count(p.entry))
                err(errors, stage_id + ".placements." + p.id, "entry",
                    "graph '" + p.entry + "' not found");

        s.stages.emplace(stage_id, std::move(stage));
    }

    if (!s.entry_stage.empty() && !s.stages.count(s.entry_stage)) {
        err(errors, "scene", "entry_stage", "'" + s.entry_stage + "' not found");
        s.entry_stage.clear();
    }
}

// --------------------------------------------------------- locale merger

inline std::unordered_map<std::string, std::string>
merge_locales(const std::vector<std::string>& jsons,
              const std::string& preferred,
              std::vector<LoadError>& errors)
{
    struct Entry { std::string locale; std::unordered_map<std::string, std::string> table; };
    std::vector<Entry> entries;

    for (const auto& jt : jsons) {
        auto jp = nlohmann::json::parse(jt, nullptr, false);
        if (jp.is_discarded()) {
            const std::string why = json_parse_failure_reason(jt);
            DIALOG_LOG(Error, "locale JSON parse failed (" << jt.size() << " bytes): " << why);
            err(errors, "", "", "locale: invalid JSON");
            continue;
        }
        Entry e;
        e.locale = jp.value("locale", "");
        if (jp.contains("strings") && jp["strings"].is_object()) {
            for (auto it = jp["strings"].begin(); it != jp["strings"].end(); ++it)
                if (it.value().is_string()) e.table[it.key()] = it.value().get<std::string>();
        }
        entries.push_back(std::move(e));
    }

    std::unordered_map<std::string, std::string> out;
    for (auto& e : entries) if (e.locale == preferred)
        for (auto& [k, v] : e.table) out.emplace(k, v);
    for (auto& e : entries) if (e.locale != preferred)
        for (auto& [k, v] : e.table) out.emplace(k, v);
    return out;
}

inline void log_scene_summary(const SceneData& s, const std::vector<LoadError>& errors,
                              const std::string& current_stage)
{
    DIALOG_LOG(Info, "scene '" << s.id << "' loaded: entry_stage='" << s.entry_stage
                     << "' stages=" << s.stages.size() << " nodes=" << s.nodes.size()
                     << " choices=" << s.choices.size() << " exprs=" << s.exprs.size()
                     << " vars=" << s.var_types.size()
                     << " characters=" << s.characters.size()
                     << " load_errors=" << errors.size());

    if (s.entry_stage.empty())
        DIALOG_LOG(Error, "no usable entry_stage -> every approach() will fail with "
                          "'no active stage'");
    if (s.stages.empty())
        DIALOG_LOG(Error, "scene has no stages");

    for (const auto& [sid, st] : s.stages) {
        DIALOG_LOG(Info, "  stage '" << sid << "': placements=" << st.placements.size()
                         << " graphs_with_entry=" << st.graph_entries.size());
        for (const auto& p : st.placements) {
            auto g = st.graph_entries.find(p.entry);
            if (g == st.graph_entries.end())
                DIALOG_LOG(Error, "    placement '" << p.id << "' (character '"
                                  << p.character << ") -> graph '" << p.entry
                                  << "' has NO ENTRY -> approach('" << p.id
                                  << "') will fail");
            else
                DIALOG_LOG(Debug, "    placement '" << p.id << "' -> graph '" << p.entry
                                  << "' start " << node_label(s, g->second));
        }
        if (st.placements.empty())
            DIALOG_LOG(Warn, "  stage '" << sid << "' has no valid placements");
    }

    if (!errors.empty())
        DIALOG_LOG(Error, "scene loaded WITH " << errors.size()
                          << " ERROR(S) - ok() is false; nodes affected by them can "
                             "dead-end into Ended/Error at runtime. current_stage='"
                          << current_stage << "'");
}

} // namespace detail

// ---------------------------------------------------------------- runtime

class Runtime {
public:
    // Default-constructed runtime has no scene loaded. All methods are safe to
    // call on it (approach() yields an Error frame, advance()/choose() are
    // no-ops, getters return false/0). Call load_scene()/load_scene_files()
    // to initialise it later.
    Runtime() = default;

    Runtime(const std::string& scene_json,
            const std::vector<std::string>& locale_jsons,
            const std::string& preferred_locale = "")
    {
        load_scene(scene_json, locale_jsons, preferred_locale);
    }

    // Override how load_scene_files() reads files. The callback receives a
    // filename and must return the file contents.
    using LoadFunc = std::function<std::string(const std::string&)>;

    void SetLoadFunc(LoadFunc func) {
        load_func_ = std::move(func);
    }
    

    // (Re)initialise from in-memory JSON. Discards any previous scene, errors
    // and playback/variable state. Returns ok().
    bool load_scene(const std::string& scene_json,
                    const std::vector<std::string>& locale_jsons,
                    const std::string& preferred_locale = "")
    {
        {
            std::ostringstream sizes;
            for (const auto& l : locale_jsons) sizes << ' ' << l.size() << 'B';
            DIALOG_LOG(Info, "load_scene [" DIALOG_PLATFORM_NAME "]: scene=" << scene_json.size()
                             << "B locales=" << locale_jsons.size() << " [" << sizes.str()
                             << " ] preferred='" << preferred_locale << "'");
        }
        scene_ = detail::SceneData{};
        errors_.clear();
        visible_choices_.clear();
        frame_ = Frame{};

#ifdef DIALOG_HAS_EXCEPTIONS
        try {
#endif
            auto strings = detail::merge_locales(locale_jsons, preferred_locale, errors_);
            detail::compile_scene(scene_, scene_json, strings, errors_);
#ifdef DIALOG_HAS_EXCEPTIONS
        } catch (const std::exception& ex) {
            // Re-thrown: behaviour is unchanged, but a host that swallows
            // exceptions at a JNI/JS boundary will at least leave a trace.
            DIALOG_LOG(Error, "load_scene: EXCEPTION during compile: " << ex.what()
                              << " (scene is half-built, " << errors_.size()
                              << " load error(s) so far)");
            throw;
        }
#endif

        current_stage_ = scene_.entry_stage;
        current_       = detail::kInvalid;
        bools_         = scene_.bool_defaults;
        ints_          = scene_.int_defaults;
        loaded_        = true;
        detail::log_scene_summary(scene_, errors_, current_stage_);
        return errors_.empty();
    }

    // (Re)initialise from files. Unreadable files are treated as empty
    // (same behaviour as load_files()), which surfaces as a LoadError.
    bool load_scene_files(const std::string& scene_path,
                          const std::vector<std::string>& locale_paths,
                          const std::string& preferred_locale = "")
    {
        std::vector<std::string> locales;
        locales.reserve(locale_paths.size());
        for (const auto& p : locale_paths) locales.push_back(load_func_(p));
        return load_scene(load_func_(scene_path), locales, preferred_locale);
    }

    // True once a scene load has been attempted (successfully or not).
    bool loaded() const { return loaded_; }

    bool ok() const { return errors_.empty(); }
    const std::vector<LoadError>& errors() const { return errors_; }

    void approach(const std::string& placement_id);
    void advance();
    void choose(uint32_t index);
    void cancel();

    const Frame&       frame() const { return frame_; }
    const std::string& stage() const { return current_stage_; }

    bool    get_bool(const std::string& n) const;
    int64_t get_int (const std::string& n) const;
    void    set_bool(const std::string& n, bool v);
    void    set_int (const std::string& n, int64_t v);

    std::string save() const;
    bool        load(const std::string& json, std::vector<std::string>& warnings);

private:
    void    run_();
    void    set_node_(detail::NodeId id);
    void    apply_set_(const detail::Node& n);
    bool    eval_expr_(detail::ExprId id) const;
    int64_t eval_int_(detail::ExprId id) const;
    bool    eval_cmp_(detail::Expr::Kind op, detail::ExprId l, detail::ExprId r) const;

    detail::SceneData         scene_;
    std::string               current_stage_;
    detail::NodeId            current_ = detail::kInvalid;
    Frame                     frame_;
    std::unordered_map<std::string, bool>    bools_;
    std::unordered_map<std::string, int64_t> ints_;
    std::vector<uint32_t>     visible_choices_;
    std::vector<LoadError>    errors_;
    bool                      loaded_ = false;

    LoadFunc load_func_ = [](const std::string& filename) -> std::string {
        std::ifstream f(filename, std::ios::binary);
        std::ostringstream ss;
        ss << f.rdbuf();
        return ss.str();
    };
};

// ----------------------------------------------------------- file helper

inline Runtime load_files(const std::string& scene_path,
                          const std::vector<std::string>& locale_paths,
                          const std::string& preferred_locale = "")
{
    Runtime r;
    r.load_scene_files(scene_path, locale_paths, preferred_locale);
    return r;
}

// ============================================================ implementation

inline void Runtime::approach(const std::string& placement_id) {
    DIALOG_LOG(Debug, "approach('" << placement_id << "'): stage='" << current_stage_
                      << "' loaded=" << loaded_ << " load_errors=" << errors_.size()
                      << " active_node=" << detail::node_label(scene_, current_));
    if (current_ != detail::kInvalid)
        DIALOG_LOG(Info, "approach('" << placement_id << "') while a conversation is "
                         "ACTIVE at " << detail::node_label(scene_, current_)
                         << " - restarting from the entry node (is the host calling "
                            "approach() every tick/frame?)");

    auto sit = scene_.stages.find(current_stage_);
    if (sit == scene_.stages.end()) {
        frame_ = Frame{}; frame_.kind = FrameKind::Error;
        frame_.error = "no active stage";
        DIALOG_LOG(Error, "approach('" << placement_id << "') -> Error: no active stage "
                          "(current_stage='" << current_stage_ << "', loaded=" << loaded_
                          << ", stages=" << scene_.stages.size() << ", entry_stage='"
                          << scene_.entry_stage << "', load_errors=" << errors_.size()
                          << "). Scene probably failed to load - check ok()/errors().");
        return;
    }
    auto& stage = sit->second;

    const detail::Placement* found = nullptr;
    for (const auto& p : stage.placements)
        if (p.id == placement_id) { found = &p; break; }

    if (!found) {
        frame_ = Frame{}; frame_.kind = FrameKind::Error;
        frame_.error = "no such placement: " + placement_id;
        std::string have;
        for (const auto& p : stage.placements) have += " '" + p.id + "'";
        DIALOG_LOG(Error, "approach('" << placement_id << "') -> Error: no such placement "
                          "in stage '" << current_stage_ << "'. Known placements:"
                          << (have.empty() ? " <none>" : have));
        return;
    }
    auto git = stage.graph_entries.find(found->entry);
    if (git == stage.graph_entries.end()) {
        frame_ = Frame{}; frame_.kind = FrameKind::Error;
        frame_.error = "entry graph not found: " + found->entry;
        std::string have;
        for (const auto& g : stage.graph_entries) have += " '" + g.first + "'";
        DIALOG_LOG(Error, "approach('" << placement_id << "') -> Error: entry graph '"
                          << found->entry << "' not found in stage '" << current_stage_
                          << "' (graph missing or its 'start' failed to resolve). "
                             "Graphs with entry:" << (have.empty() ? " <none>" : have));
        return;
    }
    DIALOG_LOG(Info, "approach('" << placement_id << "') -> graph '" << found->entry
                     << "' start " << detail::node_label(scene_, git->second));
    set_node_(git->second);
}

inline void Runtime::advance() {
    DIALOG_LOG(Debug, "advance() at " << detail::node_label(scene_, current_)
                      << " (frame=" << detail::kind_name(frame_.kind) << ")");
    if (current_ == detail::kInvalid) {
        DIALOG_LOG(Warn, "advance() IGNORED: no active node (frame is "
                         << detail::kind_name(frame_.kind) << "). Conversation already "
                            "ended/cancelled/errored, or approach() never succeeded.");
        return;
    }
    const auto& n = scene_.nodes[current_];
    if (n.type != detail::NodeType::Line) {
        frame_ = Frame{}; frame_.kind = FrameKind::Error;
        frame_.error = "advance() not on a Line";
        DIALOG_LOG(Error, "advance() -> Error: current node "
                          << detail::node_label(scene_, current_) << " is a "
                          << detail::node_type_name(n.type) << ", not a Line");
        return;
    }
    if (n.next == detail::kInvalid)
        DIALOG_LOG(Info, "advance(): " << detail::node_label(scene_, current_)
                         << " has no 'next' -> conversation ends");
    set_node_(n.next);
}

inline void Runtime::choose(uint32_t i) {
    DIALOG_LOG(Debug, "choose(" << i << ") at " << detail::node_label(scene_, current_)
                      << " visible=" << visible_choices_.size());
    if (current_ == detail::kInvalid) {
        DIALOG_LOG(Warn, "choose(" << i << ") IGNORED: no active node (frame is "
                         << detail::kind_name(frame_.kind) << ")");
        return;
    }
    const auto& n = scene_.nodes[current_];
    if (n.type != detail::NodeType::Choice) {
        frame_ = Frame{}; frame_.kind = FrameKind::Error;
        frame_.error = "choose() not on a Choice";
        DIALOG_LOG(Error, "choose(" << i << ") -> Error: current node "
                          << detail::node_label(scene_, current_) << " is a "
                          << detail::node_type_name(n.type) << ", not a Choice");
        return;
    }
    if (i >= visible_choices_.size()) {
        frame_ = Frame{}; frame_.kind = FrameKind::Error;
        frame_.error = "choice index out of range";
        DIALOG_LOG(Error, "choose(" << i << ") -> Error: index out of range, only "
                          << visible_choices_.size() << " visible choice(s)");
        return;
    }
    const auto& c = scene_.choices[visible_choices_[i]];
    DIALOG_LOG(Debug, "choose(" << i << ") '" << detail::clip(c.text) << "' -> "
                      << detail::node_label(scene_, c.target));
    if (c.target == detail::kInvalid)
        DIALOG_LOG(Warn, "choose(" << i << "): target is unresolved (load error) "
                         "-> conversation will end");
    set_node_(c.target);
}

inline void Runtime::cancel() {
    DIALOG_LOG(Info, "cancel() at " << detail::node_label(scene_, current_)
                     << " (frame was " << detail::kind_name(frame_.kind)
                     << ") -> Ended");
    current_ = detail::kInvalid;
    visible_choices_.clear();
    frame_ = Frame{};
    frame_.kind = FrameKind::Ended;
}

inline bool Runtime::get_bool(const std::string& n) const {
    auto it = bools_.find(n);
    if (it == bools_.end()) {
        DIALOG_LOG(Warn, "get_bool('" << n << "'): no such bool variable -> false");
        return false;
    }
    return it->second;
}
inline int64_t Runtime::get_int(const std::string& n) const {
    auto it = ints_.find(n);
    if (it == ints_.end()) {
        DIALOG_LOG(Warn, "get_int('" << n << "'): no such int variable -> 0");
        return 0;
    }
    return it->second;
}
inline void Runtime::set_bool(const std::string& n, bool v) {
    auto vt = scene_.var_types.find(n);
    if (vt == scene_.var_types.end())
        DIALOG_LOG(Warn, "set_bool('" << n << "', " << v << "): variable is NOT "
                         "declared in the scene - no dialogue condition can see it");
    else if (vt->second != detail::VarType::Bool)
        DIALOG_LOG(Warn, "set_bool('" << n << "', " << v << "): variable is declared "
                         "as INT - dialogue conditions will not see this value");
    else
        DIALOG_LOG(Debug, "set_bool('" << n << "', " << v << ")");
    bools_[n] = v;
}
inline void Runtime::set_int(const std::string& n, int64_t v) {
    auto vt = scene_.var_types.find(n);
    if (vt == scene_.var_types.end())
        DIALOG_LOG(Warn, "set_int('" << n << "', " << v << "): variable is NOT "
                         "declared in the scene - no dialogue condition can see it");
    else if (vt->second != detail::VarType::Int)
        DIALOG_LOG(Warn, "set_int('" << n << "', " << v << "): variable is declared "
                         "as BOOL - dialogue conditions will not see this value");
    else
        DIALOG_LOG(Debug, "set_int('" << n << "', " << v << ")");
    ints_[n] = v;
}

inline int64_t Runtime::eval_int_(detail::ExprId id) const {
    const auto& e = scene_.exprs[id];
    switch (e.kind) {
        case detail::Expr::Kind::LitInt: return e.ival;
        case detail::Expr::Kind::VarInt: return get_int(e.var);
        default: return 0;
    }
}

inline bool Runtime::eval_cmp_(detail::Expr::Kind op, detail::ExprId l, detail::ExprId r) const {
    using K = detail::Expr::Kind;
    if (detail::expr_result_type(scene_.exprs[l].kind) == detail::VarType::Bool) {
        const bool a = eval_expr_(l), b = eval_expr_(r);
        return op == K::Eq ? a == b : op == K::Ne ? a != b : false;
    }
    const int64_t a = eval_int_(l), b = eval_int_(r);
    switch (op) {
        case K::Eq: return a == b;
        case K::Ne: return a != b;
        case K::Lt: return a <  b;
        case K::Le: return a <= b;
        case K::Gt: return a >  b;
        case K::Ge: return a >= b;
        default:    return false;
    }
}

inline bool Runtime::eval_expr_(detail::ExprId id) const {
    using K = detail::Expr::Kind;
    if (id >= scene_.exprs.size())
        DIALOG_LOG(Error, "eval_expr_: expr id " << id << " out of range ("
                          << scene_.exprs.size() << " exprs) - a condition failed to "
                             "compile (see load errors); undefined behaviour follows");
    const auto& e = scene_.exprs[id];
    switch (e.kind) {
        case K::LitBool: return e.bval;
        case K::VarBool: return get_bool(e.var);
        case K::Not:     return !eval_expr_(e.kids[0]);
        case K::And:
            for (auto k : e.kids) if (!eval_expr_(k)) return false;
            return true;
        case K::Or:
            for (auto k : e.kids) if (eval_expr_(k)) return true;
            return false;
        case K::Eq: case K::Ne:
        case K::Lt: case K::Le:
        case K::Gt: case K::Ge: return eval_cmp_(e.kind, e.kids[0], e.kids[1]);
        case K::LitInt: return e.ival != 0;
        case K::VarInt: return get_int(e.var) != 0;
    }
    return false;
}

inline void Runtime::apply_set_(const detail::Node& n) {
    using S = detail::SetOp;
    auto vt = scene_.var_types.find(n.set_var);
    if (vt == scene_.var_types.end()) {
        DIALOG_LOG(Warn, "apply_set_: Set node has unknown/empty variable '"
                         << n.set_var << "' (load error) - skipped");
        return;
    }

    if (vt->second == detail::VarType::Bool) {
        switch (n.set_op) {
            case S::Assign: bools_[n.set_var] = n.set_b; break;
            case S::Not:    bools_[n.set_var] = !get_bool(n.set_var); break;
            default: break;
        }
    } else {
        switch (n.set_op) {
            case S::Assign: ints_[n.set_var] = n.set_i; break;
            case S::Add:    ints_[n.set_var] = get_int(n.set_var) + n.set_i; break;
            case S::Sub:    ints_[n.set_var] = get_int(n.set_var) - n.set_i; break;
            default: break;
        }
    }
}

inline void Runtime::set_node_(detail::NodeId id) { current_ = id; run_(); }

inline void Runtime::run_() {
    using NT = detail::NodeType;
    uint32_t           steps = 0;
    detail::NodeId     prev  = detail::kInvalid;   // node that handed us current_
    const char*        via   = "";                 // which field it used
    for (;;) {
        if (++steps == 10000)
            DIALOG_LOG(Error, "run_: 10000 silent steps without reaching a "
                              "Line/Choice/End - probable cycle of If/Set/Jump nodes "
                              "near " << detail::node_label(scene_, current_));

        if (current_ == detail::kInvalid) {
            frame_ = Frame{}; frame_.kind = FrameKind::Ended;
            if (prev == detail::kInvalid) {
                DIALOG_LOG(Debug, "run_: -> Ended (entered with no node)");
            } else {
                const auto& p = scene_.nodes[prev];
                // A Line without 'next' is a legitimate way to finish; anything
                // else running off the graph is almost certainly a data bug.
                if (p.type == NT::Line)
                    DIALOG_LOG(Info, "run_: -> Ended after last Line "
                                     << detail::node_label(scene_, prev)
                                     << " (no 'next')");
                else
                    DIALOG_LOG(Warn, "run_: -> Ended because "
                                     << detail::node_type_name(p.type) << " node "
                                     << detail::node_label(scene_, prev) << " has no '"
                                     << via << "' target (unset, or unresolved due to a "
                                        "load error; load_errors=" << errors_.size() << ")");
            }
            return;
        }
        if (current_ >= scene_.nodes.size()) {
            DIALOG_LOG(Error, "run_: node id " << current_ << " out of range ("
                              << scene_.nodes.size() << " nodes) - scene not loaded?");
            frame_ = Frame{}; frame_.kind = FrameKind::Ended;
            return;
        }
        const auto& n = scene_.nodes[current_];

        if (n.type == NT::Line) {
            visible_choices_.clear();
            frame_ = Frame{};
            frame_.kind    = FrameKind::Line;
            frame_.speaker = n.speaker;
            frame_.text    = n.text;
            if (!n.speaker.empty()) {
                auto it = scene_.characters.find(n.speaker);
                if (it == scene_.characters.end())
                    DIALOG_LOG(Warn, "run_: speaker '" << n.speaker << "' at "
                                     << detail::node_label(scene_, current_)
                                     << " is not in 'characters' - using the id as name");
                frame_.speaker_name = (it != scene_.characters.end()) ? it->second : n.speaker;
            }
            DIALOG_LOG(Debug, "run_: -> Line " << detail::node_label(scene_, current_)
                              << " speaker='" << n.speaker << "' text='"
                              << detail::clip(n.text) << "'");
            return;
        }

        if (n.type == NT::Choice) {
            frame_ = Frame{};
            visible_choices_.clear();
            std::string hidden;
            for (uint32_t i = 0; i < n.choice_count; ++i) {
                const auto& c = scene_.choices[n.choice_begin + i];
                if (c.cond != detail::kNoExpr && !eval_expr_(c.cond)) {
                    hidden += " [" + detail::clip(c.text, 24) + "]";
                    continue;
                }
                visible_choices_.push_back(n.choice_begin + i);
                frame_.choices.push_back(c.text);
            }
            if (visible_choices_.empty()) {
                frame_.kind = FrameKind::Error;
                frame_.error = "no visible choices";
                DIALOG_LOG(Error, "run_: -> Error 'no visible choices' at "
                                  << detail::node_label(scene_, current_) << ": "
                                  << n.choice_count << " choice(s) defined, "
                                  << (n.choice_count ? "all hidden by conditions:"
                                                     : "none defined (bad 'choices' array?)")
                                  << hidden);
                current_ = detail::kInvalid;
                return;
            }
            frame_.kind = FrameKind::Choice;
            DIALOG_LOG(Debug, "run_: -> Choice " << detail::node_label(scene_, current_)
                              << " visible=" << visible_choices_.size() << "/"
                              << n.choice_count
                              << (hidden.empty() ? "" : " hidden:") << hidden);
            return;
        }

        if (n.type == NT::If) {
            if (n.cond == detail::kNoExpr)
                DIALOG_LOG(Error, "run_: If node " << detail::node_label(scene_, current_)
                                  << " has NO compiled condition (load error)");
            const bool c = eval_expr_(n.cond);
            DIALOG_LOG(Debug, "run_: If " << detail::node_label(scene_, current_)
                              << " cond=" << (c ? "true" : "false") << " -> "
                              << detail::node_label(scene_, c ? n.then_ : n.else_));
            prev = current_; via = c ? "then" : "else";
            current_ = c ? n.then_ : n.else_;
            continue;
        }

        if (n.type == NT::Set) {
            apply_set_(n);
            DIALOG_LOG(Debug, "run_: Set " << detail::node_label(scene_, current_)
                              << " var='" << n.set_var << "' -> "
                              << detail::node_label(scene_, n.next));
            prev = current_; via = "next";
            current_ = n.next;
            continue;
        }

        if (n.type == NT::Jump) {
            DIALOG_LOG(Debug, "run_: Jump " << detail::node_label(scene_, current_)
                              << " -> " << detail::node_label(scene_, n.next));
            prev = current_; via = "target";
            current_ = n.next;
            continue;
        }

        // NT::End
        DIALOG_LOG(Info, "run_: -> Ended at End node "
                         << detail::node_label(scene_, current_));
        current_ = detail::kInvalid;
        frame_ = Frame{}; frame_.kind = FrameKind::Ended;
        return;
    }
}

// -------------------------------------------------------------- save/load

inline std::string Runtime::save() const {
    nlohmann::json j;
    j["version"] = 1;
    j["scene"]   = scene_.id;
    j["stage"]   = current_stage_;
    j["node"]    = (current_ != detail::kInvalid && current_ < scene_.node_paths.size())
                 ? scene_.node_paths[current_] : std::string{};

    for (auto& [k, v] : bools_) j["bools"][k] = v;
    for (auto& [k, v] : ints_)  j["ints"][k]  = v;
    return j.dump();
}

inline bool Runtime::load(const std::string& json,
                          std::vector<std::string>& warnings)
{
    auto j = nlohmann::json::parse(json, nullptr, false);
    if (j.is_discarded())      { detail::warn(warnings, "save: invalid JSON"); return false; }
    if (j.value("version", 0) != 1) { detail::warn(warnings, "save: bad version"); return false; }
    if (j.value("scene", "") != scene_.id) { detail::warn(warnings, "save: scene mismatch"); return false; }

    const std::string stage_id = j.value("stage", "");
    if (!scene_.stages.count(stage_id)) { detail::warn(warnings, "save: stage missing"); return false; }
    current_stage_ = stage_id;

    bools_ = scene_.bool_defaults;
    ints_  = scene_.int_defaults;

    if (j.contains("bools") && j["bools"].is_object())
        for (auto it = j["bools"].begin(); it != j["bools"].end(); ++it) {
            if (!scene_.bool_defaults.count(it.key())) {
                detail::warn(warnings, "save: unknown bool var '" + it.key() + "'");
                continue;
            }
            if (!it.value().is_boolean()) {
                detail::warn(warnings, "save: bool var '" + it.key() + "' wrong type");
                continue;
            }
            bools_[it.key()] = it.value().get<bool>();
        }
    if (j.contains("ints") && j["ints"].is_object())
        for (auto it = j["ints"].begin(); it != j["ints"].end(); ++it) {
            if (!scene_.int_defaults.count(it.key())) {
                detail::warn(warnings, "save: unknown int var '" + it.key() + "'");
                continue;
            }
            if (!it.value().is_number_integer()) {
                detail::warn(warnings, "save: int var '" + it.key() + "' wrong type");
                continue;
            }
            ints_[it.key()] = it.value().get<int64_t>();
        }

    current_ = detail::kInvalid;
    visible_choices_.clear();
    frame_ = Frame{};
    frame_.kind = FrameKind::Ended;

    const std::string path = j.value("node", "");
    if (!path.empty()) {
        auto it = scene_.path_to_node.find(path);
        if (it != scene_.path_to_node.end()) {
            current_ = it->second;
        } else {
            const size_t d1 = path.find('.');
            const size_t d2 = path.rfind('.');
            if (d1 != std::string::npos && d2 != d1) {
                const std::string stg = path.substr(0, d1);
                const std::string grp = path.substr(d1 + 1, d2 - d1 - 1);
                auto sit = scene_.stages.find(stg);
                if (sit != scene_.stages.end()) {
                    auto git = sit->second.graph_entries.find(grp);
                    if (git != sit->second.graph_entries.end()) {
                        current_ = git->second;
                        detail::warn(warnings, "save: node missing, using graph start");
                    } else {
                        detail::warn(warnings, "save: graph missing, no active conversation");
                    }
                } else {
                    detail::warn(warnings, "save: stage in path missing");
                }
            } else {
                detail::warn(warnings, "save: malformed node path");
            }
        }
    }

    DIALOG_LOG(Info, "load(): restored stage='" << current_stage_ << "' node="
                     << detail::node_label(scene_, current_));
    if (current_ != detail::kInvalid) run_();
    DIALOG_LOG(Info, "load(): resulting frame=" << detail::kind_name(frame_.kind)
                     << " warnings=" << warnings.size());
    return true;
}

} // namespace dialog