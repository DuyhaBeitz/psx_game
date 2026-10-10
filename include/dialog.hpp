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

#pragma once
#include <cstdint>
#include <fstream>
#include <functional>
#include <limits>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#include "nlohmann/json.hpp"

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

using NodeId = uint32_t;
using ExprId = uint32_t;
constexpr NodeId kInvalid = std::numeric_limits<NodeId>::max();
constexpr ExprId kNoExpr  = std::numeric_limits<ExprId>::max();

enum class VarType { Bool, Int };
enum class SetOp   { Assign, Add, Sub, Not };
enum class NodeType { Line, Choice, If, Set, Jump, End };

inline void err(std::vector<LoadError>& e, std::string path,
                std::string field, std::string msg)
{
    e.push_back({std::move(path), std::move(field), std::move(msg)});
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
}

// --------------------------------------------------------- scene compiler

inline void compile_scene(SceneData& s, const std::string& json,
                          const std::unordered_map<std::string, std::string>& strings,
                          std::vector<LoadError>& errors)
{
    auto j = nlohmann::json::parse(json, nullptr, false);
    if (j.is_discarded()) { err(errors, "", "", "scene: invalid JSON"); return; }

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
        if (jp.is_discarded()) { err(errors, "", "", "locale: invalid JSON"); continue; }
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

    // (Re)initialise from in-memory JSON. Discards any previous scene, errors
    // and playback/variable state. Returns ok().
    bool load_scene(const std::string& scene_json,
                    const std::vector<std::string>& locale_jsons,
                    const std::string& preferred_locale = "")
    {
        scene_ = detail::SceneData{};
        errors_.clear();
        visible_choices_.clear();
        frame_ = Frame{};

        auto strings = detail::merge_locales(locale_jsons, preferred_locale, errors_);
        detail::compile_scene(scene_, scene_json, strings, errors_);

        current_stage_ = scene_.entry_stage;
        current_       = detail::kInvalid;
        bools_         = scene_.bool_defaults;
        ints_          = scene_.int_defaults;
        loaded_        = true;
        return errors_.empty();
    }

    // (Re)initialise from files. Unreadable files are treated as empty
    // (same behaviour as load_files()), which surfaces as a LoadError.
    bool load_scene_files(const std::string& scene_path,
                          const std::vector<std::string>& locale_paths,
                          const std::string& preferred_locale = "")
    {
        auto read = [](const std::string& path) -> std::string {
            std::ifstream f(path, std::ios::binary);
            std::ostringstream ss;
            ss << f.rdbuf();
            return ss.str();
        };
        std::vector<std::string> locales;
        locales.reserve(locale_paths.size());
        for (const auto& p : locale_paths) locales.push_back(read(p));
        return load_scene(read(scene_path), locales, preferred_locale);
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
    auto sit = scene_.stages.find(current_stage_);
    if (sit == scene_.stages.end()) {
        frame_ = Frame{}; frame_.kind = FrameKind::Error;
        frame_.error = "no active stage";
        return;
    }
    auto& stage = sit->second;

    const detail::Placement* found = nullptr;
    for (const auto& p : stage.placements)
        if (p.id == placement_id) { found = &p; break; }

    if (!found) {
        frame_ = Frame{}; frame_.kind = FrameKind::Error;
        frame_.error = "no such placement: " + placement_id;
        return;
    }
    auto git = stage.graph_entries.find(found->entry);
    if (git == stage.graph_entries.end()) {
        frame_ = Frame{}; frame_.kind = FrameKind::Error;
        frame_.error = "entry graph not found: " + found->entry;
        return;
    }
    set_node_(git->second);
}

inline void Runtime::advance() {
    if (current_ == detail::kInvalid) return;
    const auto& n = scene_.nodes[current_];
    if (n.type != detail::NodeType::Line) {
        frame_ = Frame{}; frame_.kind = FrameKind::Error;
        frame_.error = "advance() not on a Line";
        return;
    }
    set_node_(n.next);
}

inline void Runtime::choose(uint32_t i) {
    if (current_ == detail::kInvalid) return;
    const auto& n = scene_.nodes[current_];
    if (n.type != detail::NodeType::Choice) {
        frame_ = Frame{}; frame_.kind = FrameKind::Error;
        frame_.error = "choose() not on a Choice";
        return;
    }
    if (i >= visible_choices_.size()) {
        frame_ = Frame{}; frame_.kind = FrameKind::Error;
        frame_.error = "choice index out of range";
        return;
    }
    set_node_(scene_.choices[visible_choices_[i]].target);
}

inline void Runtime::cancel() {
    current_ = detail::kInvalid;
    visible_choices_.clear();
    frame_ = Frame{};
    frame_.kind = FrameKind::Ended;
}

inline bool Runtime::get_bool(const std::string& n) const {
    auto it = bools_.find(n); return it != bools_.end() ? it->second : false;
}
inline int64_t Runtime::get_int(const std::string& n) const {
    auto it = ints_.find(n); return it != ints_.end() ? it->second : 0;
}
inline void Runtime::set_bool(const std::string& n, bool v)    { bools_[n] = v; }
inline void Runtime::set_int (const std::string& n, int64_t v) { ints_[n]  = v; }

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
    if (vt == scene_.var_types.end()) return;

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
    for (;;) {
        if (current_ == detail::kInvalid) {
            frame_ = Frame{}; frame_.kind = FrameKind::Ended; return;
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
                frame_.speaker_name = (it != scene_.characters.end()) ? it->second : n.speaker;
            }
            return;
        }

        if (n.type == NT::Choice) {
            frame_ = Frame{};
            visible_choices_.clear();
            for (uint32_t i = 0; i < n.choice_count; ++i) {
                const auto& c = scene_.choices[n.choice_begin + i];
                if (c.cond != detail::kNoExpr && !eval_expr_(c.cond)) continue;
                visible_choices_.push_back(n.choice_begin + i);
                frame_.choices.push_back(c.text);
            }
            if (visible_choices_.empty()) {
                frame_.kind = FrameKind::Error;
                frame_.error = "no visible choices";
                current_ = detail::kInvalid;
                return;
            }
            frame_.kind = FrameKind::Choice;
            return;
        }

        if (n.type == NT::If) {
            current_ = eval_expr_(n.cond) ? n.then_ : n.else_;
            continue;
        }

        if (n.type == NT::Set) {
            apply_set_(n);
            current_ = n.next;
            continue;
        }

        if (n.type == NT::Jump) {
            current_ = n.next;
            continue;
        }

        // NT::End
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
    if (j.is_discarded())      { warnings.push_back("save: invalid JSON"); return false; }
    if (j.value("version", 0) != 1) { warnings.push_back("save: bad version"); return false; }
    if (j.value("scene", "") != scene_.id) { warnings.push_back("save: scene mismatch"); return false; }

    const std::string stage_id = j.value("stage", "");
    if (!scene_.stages.count(stage_id)) { warnings.push_back("save: stage missing"); return false; }
    current_stage_ = stage_id;

    bools_ = scene_.bool_defaults;
    ints_  = scene_.int_defaults;

    if (j.contains("bools") && j["bools"].is_object())
        for (auto it = j["bools"].begin(); it != j["bools"].end(); ++it) {
            if (!scene_.bool_defaults.count(it.key())) {
                warnings.push_back("save: unknown bool var '" + it.key() + "'");
                continue;
            }
            if (!it.value().is_boolean()) {
                warnings.push_back("save: bool var '" + it.key() + "' wrong type");
                continue;
            }
            bools_[it.key()] = it.value().get<bool>();
        }
    if (j.contains("ints") && j["ints"].is_object())
        for (auto it = j["ints"].begin(); it != j["ints"].end(); ++it) {
            if (!scene_.int_defaults.count(it.key())) {
                warnings.push_back("save: unknown int var '" + it.key() + "'");
                continue;
            }
            if (!it.value().is_number_integer()) {
                warnings.push_back("save: int var '" + it.key() + "' wrong type");
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
                        warnings.push_back("save: node missing, using graph start");
                    } else {
                        warnings.push_back("save: graph missing, no active conversation");
                    }
                } else {
                    warnings.push_back("save: stage in path missing");
                }
            } else {
                warnings.push_back("save: malformed node path");
            }
        }
    }

    if (current_ != detail::kInvalid) run_();
    return true;
}

} // namespace dialog