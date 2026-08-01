// Apollo Patcher GUI — Dear ImGui front-end over apollo_ctrl.
//
// UI parity with the `patcher` CLI:
//   - open a .savepatch  -> shows game name + code list (groups, flags)
//   - check the codes to apply
//   - per-code option dropdowns (replaces the CLI's scanf prompt)
//   - pick a target data file
//   - Apply -> runs apply_cheat_patch_code() per selection, log panel shows progress
//
// Rendering backend: GLFW + fixed-function OpenGL2 (needs only GL 1.1, portable
// across Win/Mac/Linux and GPU-less hosts).
#include <string>
#include <vector>
#include <cstdio>
#include <cstdlib>   // _putenv_s (Windows software-GL selection)
#include <fstream>
#include <mutex>

#include "imgui.h"
#include "imgui_internal.h"   // PushItemFlag + ImGuiItemFlags_MixedValue (tri-state)
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl2.h"
#include <GLFW/glfw3.h>

// Renderer: Dear ImGui's fixed-function OpenGL2 backend on a legacy (non-core)
// context. It needs only OpenGL 1.1 — the lowest common denominator available
// on every platform: a real GPU's compatibility profile, macOS's 2.1 legacy
// context, Mesa on Linux, and the always-present Microsoft software GL on
// RDP / VMs / old Windows. This 2D tool has no use for modern GL, so one
// backend and one code path serve all platforms.
#include "portable-file-dialogs.h"   // header-only native dialogs (osascript/zenity/Win32)
#include "apollo_ctrl.h"   // manages its own C linkage (and apollo.h is C++-safe)

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>   // MessageBox for visible startup errors (no console with -mwindows)
#endif

// Window icon (Windows/Linux only). Kept fully inside the guard so macOS pulls
// in neither zlib nor the icon data.
#ifndef __APPLE__
#include <zlib.h>
#include "icon_rgba_z.h"   // 256x256 RGBA, zlib-deflated (inflated at startup)
#endif

static GLFWwindow* g_window = nullptr;   // for native dialog parenting
static float       g_ui_scale = 1.0f;    // HiDPI content scale (column widths)

// ---- shared UI state -------------------------------------------------------
struct AppState {
    apollo_session_t*   session = nullptr;
    std::string         patch_path;
    std::string         target_path;
    std::string         game_name;
    std::vector<char>   selected;         // per-row checkbox
    std::vector<char>   viewer_open;      // per-row raw-code window open flag
    std::string         log;
    std::mutex          log_mtx;
    bool                backup = true;    // copy target -> target.bak before patching
    bool                scroll_log = false;
    bool                show_log = false; // log pane collapsed by default
    bool                open_apply_popup = false;
    std::string         apply_msg;

    void append_log(const char* line) {
        std::lock_guard<std::mutex> lk(log_mtx);
        log += line;
        log += '\n';
        scroll_log = true;
    }
    void close() {
        if (session) { apollo_close(session); session = nullptr; }
        selected.clear();
        viewer_open.clear();
        game_name.clear();
        patch_path.clear();
    }
};
static AppState g_app;

static void log_sink(void* ud, const char* line) {
    static_cast<AppState*>(ud)->append_log(line);
}

// ---- small helpers ---------------------------------------------------------
static const char* type_tag(int t) {
    switch (t) {
        case APOLLO_CODE_BSD:        return "BSD";
        case APOLLO_CODE_PYTHON:     return "PY";
        case APOLLO_CODE_SAVEWIZARD: return "SW";
        default:                     return "?";
    }
}
static ImVec4 type_color(int t) {
    switch (t) {
        case APOLLO_CODE_BSD:        return ImVec4(0.45f, 0.80f, 0.55f, 1.0f); // green
        case APOLLO_CODE_PYTHON:     return ImVec4(0.95f, 0.80f, 0.35f, 1.0f); // yellow
        case APOLLO_CODE_SAVEWIZARD: return ImVec4(0.45f, 0.70f, 0.95f, 1.0f); // blue
        default:                     return ImVec4(0.7f, 0.7f, 0.7f, 1.0f);
    }
}

// Returns true if every option group of a code has a selection (sel >= 0).
// The engine initialises sel to -1, so an untouched required option blocks apply.
static bool code_options_ready(apollo_code_t* c) {
    for (int g = 0; g < apollo_opt_group_count(c); ++g)
        if (apollo_opt_get_selected(c, g) < 0) return false;
    return true;
}

// True if any checked code still has an unfilled option group.
static bool has_unfilled_selection() {
    if (!g_app.session) return false;
    for (int i = 0; i < apollo_code_count(g_app.session); ++i) {
        if (!g_app.selected[i]) continue;
        if (!code_options_ready(apollo_code_at(g_app.session, i))) return true;
    }
    return false;
}

static int count_selected() {
    int n = 0;
    for (char c : g_app.selected) n += c ? 1 : 0;
    return n;
}

static void select_all(bool on) {
    for (auto& c : g_app.selected) c = on ? 1 : 0;
}

// A group parent's children are the consecutive is_child codes following it.
// Fills [begin,end) with that range (empty if the code has no children).
static void group_children(int parent, int& begin, int& end) {
    int count = apollo_code_count(g_app.session);
    begin = parent + 1;
    end = begin;
    while (end < count && apollo_code_at(g_app.session, end)->is_child) ++end;
}

// Checking any code auto-checks every [R] required code in the patch. By design
// these are prerequisite steps (e.g. decrypt/re-encrypt) that must always run.
static void auto_enable_required() {
    if (!g_app.session) return;
    for (int i = 0; i < apollo_code_count(g_app.session); ++i)
        if (apollo_code_at(g_app.session, i)->flags & APOLLO_CODE_FLAG_REQUIRED)
            g_app.selected[i] = 1;
}

static bool backup_file(const std::string& src) {
    std::ifstream in(src, std::ios::binary);
    if (!in) return false;
    std::ofstream out(src + ".bak", std::ios::binary);
    if (!out) return false;
    out << in.rdbuf();
    return true;
}

static void load_patch(const std::string& path) {
    g_app.close();
    g_app.session = apollo_open_file(path.c_str());
    if (!g_app.session) { g_app.append_log("[!] Could not open patch file"); return; }
    g_app.patch_path = path;
    g_app.game_name = apollo_game_name(g_app.session);
    g_app.selected.assign(apollo_code_count(g_app.session), 0);
    g_app.viewer_open.assign(apollo_code_count(g_app.session), 0);
    for (int i = 0; i < apollo_code_count(g_app.session); ++i)  // pre-check [DEFAULT:] codes
        g_app.selected[i] = apollo_code_at(g_app.session, i)->activated ? 1 : 0;
    char buf[512];
    snprintf(buf, sizeof buf, "Loaded %d codes from %s",
             apollo_code_count(g_app.session), path.c_str());
    g_app.append_log(buf);
}

static void apply_selected() {
    if (!g_app.session) return;
    const char* target = g_app.target_path.empty() ? nullptr : g_app.target_path.c_str();

    if (g_app.backup && target) {
        if (backup_file(g_app.target_path))
            g_app.append_log(("Backup written: " + g_app.target_path + ".bak").c_str());
        else
            g_app.append_log("[!] Backup failed (target unreadable?) — aborting.");
        if (!std::ifstream(g_app.target_path + ".bak")) {
            g_app.show_log = true;
            g_app.apply_msg = "Could not back up the target file, so nothing was patched.\n"
                              "Check the log for details.";
            g_app.open_apply_popup = true;
            return;
        }
    }

    int applied = 0, errors = 0;
    for (int i = 0; i < apollo_code_count(g_app.session); ++i) {
        if (!g_app.selected[i]) continue;
        apollo_code_t* c = apollo_code_at(g_app.session, i);
        char hdr[256];
        snprintf(hdr, sizeof hdr, "=== Applying code #%d: %s", c->id, c->name);
        g_app.append_log(hdr);
        bool ok = apollo_apply(g_app.session, c, target);
        g_app.append_log(ok ? "- OK" : "- ERROR!");
        ++applied;
        if (!ok) ++errors;
    }
    apollo_reset_vars();
    char buf[80];
    snprintf(buf, sizeof buf, "Patching completed: %d codes applied, %d error(s)", applied, errors);
    g_app.append_log(buf);

    // Result pop-up message.
    char msg[160];
    if (errors == 0) {
        snprintf(msg, sizeof msg, "All done — %d code(s) applied successfully.", applied);
    } else {
        g_app.show_log = true;   // reveal the log so the user can inspect
        snprintf(msg, sizeof msg, "%d of %d code(s) failed to apply.\nCheck the log for details.",
                 errors, applied);
    }
    g_app.apply_msg = msg;
    g_app.open_apply_popup = true;
}

// ---- native file dialogs ---------------------------------------------------
static std::string pick_file(const char* filter_ext) {
    // portable-file-dialogs shells out to osascript/zenity (or Win32), so the
    // dialog runs outside this process — avoiding GLFW's in-process Cocoa
    // breakage on macOS. Called after the frame is rendered (see main loop).
    std::vector<std::string> filters;
    if (filter_ext) {
        filters = { std::string("Apollo patch (*.") + filter_ext + ")",
                    std::string("*.") + filter_ext,
                    "All files", "*" };
    } else {
        filters = { "All files", "*" };
    }
    auto sel = pfd::open_file("Select file", "", filters).result();
    return sel.empty() ? std::string() : sel[0];
}

// Native modals are opened AFTER the ImGui frame is rendered (see the main
// loop), not from inside a widget callback — opening a modal mid-frame is the
// second macOS pitfall. Widgets just raise these intents.
static bool g_pending_open   = false;
static bool g_pending_target = false;
static void do_open_patch()    { g_pending_open = true; }
static void do_choose_target() { g_pending_target = true; }

static void process_pending_dialogs() {
    if (g_pending_open) {
        g_pending_open = false;
        std::string p = pick_file("savepatch");
        if (!p.empty()) load_patch(p);
    }
    if (g_pending_target) {
        g_pending_target = false;
        std::string p = pick_file(nullptr);
        if (!p.empty()) g_app.target_path = p;
    }
}

// ---- widgets ---------------------------------------------------------------
static void draw_code_list() {
    if (!g_app.session) {
        ImGui::TextDisabled("Open a .savepatch file to begin (File ▸ Open, or the button above).");
        return;
    }

    // Toolbar
    if (ImGui::SmallButton("Select all"))  select_all(true);
    ImGui::SameLine();
    if (ImGui::SmallButton("Select none")) select_all(false);
    ImGui::SameLine();
    ImGui::TextDisabled("|  %d selected", count_selected());

    // Columns: Code | View | Type  (mirrors the Qt tree).
    ImGuiTableFlags flags = ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg |
                            ImGuiTableFlags_BordersInnerV;
    if (ImGui::BeginTable("codes", 3, flags, ImVec2(0, ImGui::GetContentRegionAvail().y))) {
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn("Code", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("View", ImGuiTableColumnFlags_WidthFixed, 48 * g_ui_scale);
        ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 48 * g_ui_scale);
        ImGui::TableHeadersRow();

        for (int i = 0; i < apollo_code_count(g_app.session); ++i) {
            apollo_code_t* c = apollo_code_at(g_app.session, i);
            // Scope each row by the code POINTER, not the row index. ImGui's
            // TableHeadersRow() wraps every header in PushID(column_index), so a
            // header shares an ID scope with a same-index row: the "View" column
            // is index 1, so a row-index-1 (row #2) PushID(1) put its "View"
            // button in the same scope as the "View" header -> identical ID ->
            // conflict, always on row #2, regardless of code names. A pointer
            // can never equal a small column index, so scopes never coincide.
            ImGui::PushID(c);
            ImGui::TableNextRow();

            // --- col 0: checkbox + name ---
            ImGui::TableSetColumnIndex(0);
            float indent = c->is_child ? 18.0f : 0.0f;
            if (indent) ImGui::Indent(indent);
            const char* label = (c->name && c->name[0]) ? c->name : "(unnamed)";
            bool parent = c->is_parent;
            bool disabled = (c->flags & APOLLO_CODE_FLAG_DISABLED) != 0;
            if (parent || disabled)
                ImGui::PushStyleColor(ImGuiCol_Text, parent ? ImVec4(0.80f,0.80f,0.95f,1.0f)
                                                            : ImVec4(0.55f,0.55f,0.55f,1.0f));

            // Parent shows the aggregate of its children (checked / mixed /
            // unchecked) and toggling it propagates to every child — matching
            // the Qt tree's auto-tristate behaviour.
            int cb = 0, ce = 0;
            if (parent) group_children(i, cb, ce);
            int nchild = ce - cb, nchecked = 0;
            for (int j = cb; j < ce; ++j) nchecked += g_app.selected[j] ? 1 : 0;
            bool mixed = parent && nchild > 0 && nchecked > 0 && nchecked < nchild;
            bool chk = (parent && nchild > 0) ? (nchecked == nchild)
                                              : (g_app.selected[i] != 0);

            if (mixed) ImGui::PushItemFlag(ImGuiItemFlags_MixedValue, true);
            if (ImGui::Checkbox(label, &chk)) {
                g_app.selected[i] = chk ? 1 : 0;
                for (int j = cb; j < ce; ++j) g_app.selected[j] = chk ? 1 : 0;  // parent -> children
                if (chk) auto_enable_required();   // pull in all [R] codes
            }
            if (mixed) ImGui::PopItemFlag();

            if (parent || disabled) ImGui::PopStyleColor();
            if (indent) ImGui::Unindent(indent);

            // --- col 1: View button opens a raw-code window ---
            ImGui::TableSetColumnIndex(1);
            const char* body = apollo_code_text(c);
            if (body && body[0]) {
                if (ImGui::SmallButton("View")) g_app.viewer_open[i] = 1;
            }

            // --- col 2: type badge ---
            ImGui::TableSetColumnIndex(2);
            ImGui::TextColored(type_color(c->type), "%s", type_tag(c->type));

            // --- option dropdown rows (under the Code column) ---
            for (int g = 0; g < apollo_opt_group_count(c); ++g) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Indent(indent + 18.0f);
                bool unfilled = apollo_opt_get_selected(c, g) < 0;
                const char* cur = unfilled ? "<choose a value>"
                    : apollo_opt_value_name(c, g, apollo_opt_get_selected(c, g));
                bool warn = unfilled && chk;
                if (warn) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.5f, 0.4f, 1.0f));
                ImGui::SetNextItemWidth(ImGui::GetFontSize() * 16.0f);
                if (ImGui::BeginCombo(apollo_opt_tag(c, g), cur)) {
                    for (int v = 0; v < apollo_opt_value_count(c, g); ++v) {
                        bool sel = (apollo_opt_get_selected(c, g) == v);
                        if (ImGui::Selectable(apollo_opt_value_name(c, g, v), sel))
                            apollo_opt_set_selected(c, g, v);
                    }
                    ImGui::EndCombo();
                }
                if (warn) {
                    ImGui::PopStyleColor();
                    ImGui::SameLine();
                    ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.4f, 1.0f), "(required)");
                }
                ImGui::Unindent(indent + 18.0f);
            }

            ImGui::PopID();
        }
        ImGui::EndTable();
    }
}

// Modeless raw-code windows (one per code whose View button was clicked).
static void draw_code_viewers() {
    if (!g_app.session) return;
    for (int i = 0; i < apollo_code_count(g_app.session); ++i) {
        if (!g_app.viewer_open[i]) continue;
        apollo_code_t* c = apollo_code_at(g_app.session, i);
        char title[160];
        snprintf(title, sizeof title, "Code: %s##viewer%d",
                 (c->name && c->name[0]) ? c->name : "(unnamed)", i);
        bool open = true;
        ImGui::SetNextWindowSize(ImVec2(560, 400), ImGuiCond_FirstUseEver);
        if (ImGui::Begin(title, &open)) {
            ImGui::Text("Target file: %s", (c->file && c->file[0]) ? c->file : "(none)");
            ImGui::Separator();
            ImGui::BeginChild("body", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
            ImGui::TextUnformatted(apollo_code_text(c));
            ImGui::EndChild();
        }
        ImGui::End();
        if (!open) g_app.viewer_open[i] = 0;
    }
}

static void draw_menu_bar(bool* want_quit) {
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Open .savepatch...", "Ctrl+O")) do_open_patch();
            if (ImGui::MenuItem("Choose target file...")) do_choose_target();
            ImGui::Separator();
            if (ImGui::MenuItem("Quit", "Ctrl+Q")) *want_quit = true;
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Help")) {
            ImGui::MenuItem("Apollo Patcher GUI", nullptr, false, false);
            ImGui::MenuItem("Legend: SW=Save Wizard  BSD  PY=Python", nullptr, false, false);
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }
}

static void draw_main_window(bool* want_quit) {
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->WorkPos);
    ImGui::SetNextWindowSize(vp->WorkSize);
    ImGui::Begin("Apollo Patcher", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                 ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_MenuBar);

    draw_menu_bar(want_quit);

    // --- file rows ---
    if (ImGui::Button("Open .savepatch...")) do_open_patch();
    ImGui::SameLine();
    ImGui::TextUnformatted(g_app.patch_path.empty() ? "(no patch loaded)" : g_app.patch_path.c_str());

    if (ImGui::Button("Choose target...")) do_choose_target();
    ImGui::SameLine();
    ImGui::TextUnformatted(g_app.target_path.empty() ? "(script uses patch's own target)"
                                                     : g_app.target_path.c_str());
    ImGui::Checkbox("Back up target (.bak) before patching", &g_app.backup);

    ImGui::Spacing();
    if (g_app.session)
        ImGui::TextColored(ImVec4(0.80f,0.80f,0.95f,1.0f), "Game: %s", g_app.game_name.c_str());
    ImGui::Spacing();

    // --- code list (fills remaining space above the footer) ---
    float log_h  = ImGui::GetFontSize() * 9.0f;
    float footer = ImGui::GetFrameHeightWithSpacing()          // action row
                 + ImGui::GetFrameHeightWithSpacing()          // log toggle
                 + (g_app.show_log ? log_h + ImGui::GetStyle().ItemSpacing.y : 0.0f);
    ImGui::BeginChild("list_region", ImVec2(0, -footer));
    draw_code_list();
    ImGui::EndChild();

    // --- action row ---
    bool blocked = has_unfilled_selection();
    ImGui::BeginDisabled(blocked || !g_app.session || count_selected() == 0);
    if (ImGui::Button("Apply selected", ImVec2(140, 0))) apply_selected();
    ImGui::EndDisabled();
    ImGui::SameLine();
    if (ImGui::Button("Clear log")) { std::lock_guard<std::mutex> lk(g_app.log_mtx); g_app.log.clear(); }
    if (blocked) {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.4f, 1.0f),
                           "Fill the required options on the highlighted codes first.");
    }

    // --- collapsible log ---
    if (ImGui::Button(g_app.show_log ? "- Hide log" : "+ Show log")) g_app.show_log = !g_app.show_log;
    if (g_app.show_log) {
        if (ImGui::BeginChild("log", ImVec2(0, log_h), true,
                              ImGuiWindowFlags_HorizontalScrollbar)) {
            std::lock_guard<std::mutex> lk(g_app.log_mtx);
            ImGui::TextUnformatted(g_app.log.c_str());
            if (g_app.scroll_log) { ImGui::SetScrollHereY(1.0f); g_app.scroll_log = false; }
        }
        ImGui::EndChild();
    }

    // --- apply result popup ---
    if (g_app.open_apply_popup) { ImGui::OpenPopup("Apply"); g_app.open_apply_popup = false; }
    if (ImGui::BeginPopupModal("Apply", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextUnformatted(g_app.apply_msg.c_str());
        ImGui::Spacing();
        if (ImGui::Button("OK", ImVec2(120, 0))) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    ImGui::End();
}

static void apply_style() {
    // Cosmetic rounding only — no size scaling, so everything stays at
    // ImGui's default dimensions.
    ImGuiStyle& s = ImGui::GetStyle();
    s.WindowRounding    = 6.0f;
    s.FrameRounding     = 4.0f;
    s.GrabRounding      = 4.0f;
    s.ScrollbarRounding = 4.0f;
}

// Title-bar / taskbar icon for Windows & Linux. On macOS glfwSetWindowIcon is
// unsupported (the Dock icon comes from the .app bundle's .icns instead).
static void set_window_icon(GLFWwindow* window) {
#ifndef __APPLE__
    std::vector<unsigned char> rgba(apollo_icon_rgba_size);
    uLongf out = apollo_icon_rgba_size;
    if (uncompress(rgba.data(), &out, apollo_icon_rgba_z, apollo_icon_rgba_z_len) == Z_OK
        && out == apollo_icon_rgba_size) {
        GLFWimage img;
        img.width  = apollo_icon_w;
        img.height = apollo_icon_h;
        img.pixels = rgba.data();          // GLFW copies the pixels
        glfwSetWindowIcon(window, 1, &img);
    }
#else
    (void)window;
#endif
}

// Last message from GLFW, shown to the user if startup fails (no console with
// -mwindows, so failures would otherwise be silent).
static std::string g_glfw_error;
static void glfw_error_cb(int code, const char* desc) {
    char buf[512];
    snprintf(buf, sizeof buf, "GLFW error %d: %s", code, desc ? desc : "(unknown)");
    g_glfw_error = buf;
    fprintf(stderr, "%s\n", buf);
}
static void fatal(const std::string& msg) {
#ifdef _WIN32
    MessageBoxA(nullptr, msg.c_str(), "Apollo Patcher — startup error", MB_ICONERROR | MB_OK);
#else
    fprintf(stderr, "%s\n", msg.c_str());
#endif
}

int main(int, char**) {
    apollo_set_log_sink(log_sink, &g_app);

#ifdef _WIN32
    // Use the bundled Mesa software renderer (llvmpipe). Over RDP and on GPU-less
    // hosts the system OpenGL driver reports no OpenGL at all (WGL: "driver does
    // not support OpenGL"), so GLFW can't create a context. GLFW loads
    // opengl32.dll from the exe's own directory first, so the Mesa DLL shipped
    // next to the .exe wins and renders in software (presented via GDI, which
    // works over RDP). Harmless if a real system driver is used (it ignores it).
    _putenv_s("GALLIUM_DRIVER", "llvmpipe");
#endif

    glfwSetErrorCallback(glfw_error_cb);
    if (!glfwInit()) { fatal("Failed to initialize GLFW.\n\n" + g_glfw_error); return 1; }
    // No context hints: GLFW's default legacy/compatibility context is what the
    // fixed-function opengl2 backend needs, on every platform.
    GLFWwindow* window = glfwCreateWindow(860, 820, "Apollo Patcher", nullptr, nullptr);
    if (!window) {
        fatal("Could not create the application window / OpenGL context.\n\n" + g_glfw_error);
        glfwTerminate();
        return 1;
    }
    g_window = window;
    set_window_icon(window);   // Windows/Linux title-bar & taskbar icon
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;   // don't litter the CWD with imgui.ini

    io.Fonts->AddFontDefault();   // ImGui's default 13px font, no HiDPI upscaling

    ImGui::StyleColorsDark();
    apply_style();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL2_Init();

    bool want_quit = false;
    while (!glfwWindowShouldClose(window) && !want_quit) {
        glfwPollEvents();
        ImGui_ImplOpenGL2_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // keyboard shortcuts
        if (ImGui::IsKeyDown(ImGuiMod_Ctrl) && ImGui::IsKeyPressed(ImGuiKey_O, false)) do_open_patch();
        if (ImGui::IsKeyDown(ImGuiMod_Ctrl) && ImGui::IsKeyPressed(ImGuiKey_Q, false)) want_quit = true;

        draw_main_window(&want_quit);
        draw_code_viewers();

        ImGui::Render();
        int w, h; glfwGetFramebufferSize(window, &w, &h);
        glViewport(0, 0, w, h);
        glClearColor(0.10f, 0.10f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);

        // Open any requested native dialog now — outside the ImGui frame.
        process_pending_dialogs();
    }

    g_app.close();
    ImGui_ImplOpenGL2_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
