#include "ui/MainWindow.h"
#include "io/FileManager.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <sstream>
#include <vector>

#include "ui/Theme.h"
#include "io/FileManager.h"
#include "io/GifExporter.h"
#include "io/PngExporter.h"
#include "io/ExportOptions.h"
#include "io/FileDialog.h"
#include "core/Frame.h"
#include "core/History.h"
#include <imgui.h>
#include <SDL3/SDL.h>
#include <string>

namespace Framenote {

static const char* FILTER_FRAMENOTE = "Framenote Files\0*.framenote\0All Files\0*.*\0";
static const char* FILTER_GIF       = "GIF Files\0*.gif\0All Files\0*.*\0";
static const char* FILTER_PNG       = "PNG Files\0*.png\0All Files\0*.*\0";

static std::string toLowerCopy(std::string s) {
    std::transform(
        s.begin(),
        s.end(),
        s.begin(),
        [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        }
    );

    return s;
}

static std::string pathFilename(const std::string& path) {
    try {
        return std::filesystem::path(path).filename().string();
    }
    catch (...) {
        size_t slash = path.find_last_of("/\\");
        return slash == std::string::npos ? path : path.substr(slash + 1);
    }
}

static std::string pathExtensionLower(const std::string& path) {
    try {
        return toLowerCopy(std::filesystem::path(path).extension().string());
    }
    catch (...) {
        std::string name = pathFilename(path);
        size_t dot = name.find_last_of('.');

        if (dot == std::string::npos)
            return "";

        return toLowerCopy(name.substr(dot));
    }
}

static std::string parentFolderName(const std::string& path) {
    try {
        std::string parent =
            std::filesystem::path(path).parent_path().filename().string();

        if (!parent.empty())
            return parent;
    }
    catch (...) {
    }

    return "Imported PNG Sequence";
}

static std::string joinImportErrors(const std::vector<std::string>& errors) {
    std::ostringstream oss;

    for (const auto& err : errors) {
        oss << "- " << err << "\n";
    }

    return oss.str();
}

// Captures all frames + canvas size as a HistoryEntry to pass into undo/redo
static HistoryEntry captureCurrentEntry(DocumentTab* tab) {
    HistoryEntry entry;
    entry.canvasWidth  = tab->document->canvasSize().width;
    entry.canvasHeight = tab->document->canvasSize().height;
    for (int i = 0; i < tab->document->frameCount(); ++i) {
        auto& f = tab->document->frame(i);
        Snapshot s;
        s.frameIndex   = i;
        s.bufferWidth  = f.bufferWidth();
        s.bufferHeight = f.bufferHeight();
        s.pixels       = f.pixels();
        entry.frames.push_back(std::move(s));
    }
    return entry;
}

// Applies a restored HistoryEntry — handles both drawing and resize entries
static void applyHistoryEntry(DocumentTab* tab, HistoryEntry& entry) {
    if (entry.canvasWidth > 0 &&
        (entry.canvasWidth  != tab->document->canvasSize().width ||
         entry.canvasHeight != tab->document->canvasSize().height)) {
        tab->document->setCanvasSize(entry.canvasWidth, entry.canvasHeight);
        tab->renderer->resize(entry.canvasWidth, entry.canvasHeight);
        for (int i = 0; i < tab->document->frameCount(); ++i)
            tab->document->frame(i).setVisibleSize(entry.canvasWidth, entry.canvasHeight);
    }
    for (auto& snap : entry.frames)
        tab->document->frame(snap.frameIndex).restoreBuffer(
            std::move(snap.pixels), snap.bufferWidth, snap.bufferHeight);
    tab->document->markDirty();
}

MainWindow::MainWindow(TabManager* tabManager, ToolManager* toolManager)
    : m_tabManager(tabManager)
    , m_toolManager(toolManager)
{}

void MainWindow::showError(const std::string& msg) {
    m_errorMsg        = msg;
    m_showErrorDialog = true;
}

void MainWindow::renderErrorDialog() {
    if (m_showErrorDialog) {
        ImGui::SetNextWindowPos(
            ImGui::GetMainViewport()->GetCenter(),
            ImGuiCond_Always, {0.5f, 0.5f});
        ImGui::OpenPopup("Error##errDlg");
        m_showErrorDialog = false;
    }
    ImGui::SetNextWindowPos(
        ImGui::GetMainViewport()->GetCenter(),
        ImGuiCond_Always, {0.5f, 0.5f});
    ImGui::SetNextWindowSizeConstraints({320, 0}, {600, 400});
    if (ImGui::BeginPopupModal("Error##errDlg", nullptr,
            ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextWrapped("%s", m_errorMsg.c_str());
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        if (ImGui::Button("OK", {100, 0}))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
}

bool MainWindow::doSave(DocumentTab* tab, const std::string& path) {
    std::string err;
    if (FileManager::save(*tab->document, path, err)) {
        std::string finalPath = path;
        if (finalPath.size() < 11 ||
            finalPath.substr(finalPath.size() - 11) != ".framenote")
            finalPath += ".framenote";
        tab->document->setFilePath(finalPath);
        tab->document->clearDirty();
        tab->name = finalPath.substr(finalPath.find_last_of("/\\") + 1);
        m_statusMsg = "Saved!";
        m_tabManager->recordRecentFile(finalPath, *tab->document);
        return true;
    }
    showError("Save failed:\n" + err);
    return false;
}

void MainWindow::render() {
    renderMenuBar();
    m_tabManager->render(*m_toolManager);
    m_tabManager->autosaveDirtyTabs(ImGui::GetTime());
    renderCanvasSizeDialog();
    renderExportDialog();
    renderOnionOpacityDialog();
    renderDropImportDialog();
    renderErrorDialog();

    // -- Global keyboard shortcuts ---------------------------------------------
    ImGuiIO& io = ImGui::GetIO();
    if (!io.WantTextInput) {
        bool ctrl  = io.KeyCtrl;
        bool shift = io.KeyShift;

        // Ctrl+N -- New document
        if (ctrl && ImGui::IsKeyPressed(ImGuiKey_N, false))
            m_tabManager->showNewDialog();

        // Ctrl+O -- Open file
        if (ctrl && ImGui::IsKeyPressed(ImGuiKey_O, false)) {
            std::string path = FileDialog::openFile(
                FILTER_FRAMENOTE, "Open Framenote File");
            if (!path.empty()) {
                std::string err;
                auto doc = FileManager::load(path, err);
                if (doc) {
                    std::string name = path.substr(path.find_last_of("/\\") + 1);
                    m_tabManager->openDocument(std::move(doc), name, path);

                    if (auto* opened = m_tabManager->activeTab()) {
                        m_tabManager->recordRecentFile(path, *opened->document);
                    }

                } else {
                    m_statusMsg = "Open failed: " + err;
                }
            }
        }

        // Ctrl+S -- Save
        if (ctrl && !shift && ImGui::IsKeyPressed(ImGuiKey_S, false)) {
            auto* tab = m_tabManager->activeTab();
            if (tab && tab->document->isDirty()) {
                if (tab->document->filePath().empty()) {
                    std::string path = FileDialog::saveFile(
                        FILTER_FRAMENOTE, "framenote", "Save Framenote File");
                    if (!path.empty()) {
                        std::string err;
                        if (FileManager::save(*tab->document, path, err)) {
                            tab->document->setFilePath(path);
                            tab->document->clearDirty();
                            tab->name = path.substr(path.find_last_of("/\\") + 1);
                            m_statusMsg = "Saved!";
                            m_tabManager->recordRecentFile(path, *tab->document);
                        }
                    }
                } else {
                    std::string err;
                    if (FileManager::save(*tab->document, tab->document->filePath(), err)) {
                        tab->document->clearDirty();
                        m_statusMsg = "Saved!";
                        m_tabManager->recordRecentFile(tab->document->filePath(), *tab->document);
                    }
                }
            }
        }

        // Ctrl+Shift+S -- Save As
        if (ctrl && shift && ImGui::IsKeyPressed(ImGuiKey_S, false)) {
            auto* tab = m_tabManager->activeTab();
            if (tab) {
                std::string path = FileDialog::saveFile(
                    FILTER_FRAMENOTE, "framenote", "Save As");
                if (!path.empty()) {
                    std::string err;
                    if (FileManager::save(*tab->document, path, err)) {
                        tab->document->setFilePath(path);
                        tab->document->clearDirty();
                        tab->name = path.substr(path.find_last_of("/\\") + 1);
                        m_statusMsg = "Saved!";
                        m_tabManager->recordRecentFile(path, *tab->document);
                    }
                }
            }
        }

        // C -- Canvas Size
        if (!ctrl && ImGui::IsKeyPressed(ImGuiKey_C, false)) {
            auto* tab = m_tabManager->activeTab();
            if (tab) {
                m_showCanvasSizeDialog = true;
                m_newCanvasW = tab->document->canvasSize().width;
                m_newCanvasH = tab->document->canvasSize().height;
            }
        }

        // O -- Toggle onion skin
        if (!ctrl && ImGui::IsKeyPressed(ImGuiKey_O, false)) {
            auto* tab = m_tabManager->activeTab();
            if (tab)
                tab->timeline->setOnionSkin(!tab->timeline->onionSkinEnabled());
        }

        // Ctrl+Z -- Undo
        if (ctrl && ImGui::IsKeyPressed(ImGuiKey_Z, false)) {
            auto* tab = m_tabManager->activeTab();
            if (tab && tab->history->canUndo()) {
                HistoryEntry current = captureCurrentEntry(tab);
                HistoryEntry restored = tab->history->undo(std::move(current));
                applyHistoryEntry(tab, restored);
            }
        }

        // Ctrl+Y / Ctrl+Shift+Z -- Redo
        if (ctrl && (ImGui::IsKeyPressed(ImGuiKey_Y, false) ||
                     (shift && ImGui::IsKeyPressed(ImGuiKey_Z, false)))) {
            auto* tab = m_tabManager->activeTab();
            if (tab && tab->history->canRedo()) {
                HistoryEntry current = captureCurrentEntry(tab);
                HistoryEntry restored = tab->history->redo(std::move(current));
                applyHistoryEntry(tab, restored);
            }
        }
    }
}

void MainWindow::renderMenuBar() {
    if (ImGui::BeginMainMenuBar()) {

        // -- File --------------------------------------------------------------
        if (ImGui::BeginMenu("File")) {
            auto* tab    = m_tabManager->activeTab();
            bool hasTab  = tab != nullptr;
            bool isDirty = hasTab && tab->document->isDirty();

            if (ImGui::MenuItem("New", "Ctrl+N"))
                m_tabManager->showNewDialog();

            if (ImGui::MenuItem("Open...", "Ctrl+O")) {
                std::string path = FileDialog::openFile(
                    FILTER_FRAMENOTE, "Open Framenote File");
                if (!path.empty()) {
                    std::string err;
                    auto doc = FileManager::load(path, err);
                    if (doc) {
                        std::string name = path.substr(path.find_last_of("/\\") + 1);
                        m_tabManager->openDocument(std::move(doc), name, path);

                        if (auto* opened = m_tabManager->activeTab()) {
                            m_tabManager->recordRecentFile(path, *opened->document);
                        }

                    } else {
                        m_statusMsg = "Open failed: " + err;
                    }
                }
            }

            ImGui::Separator();

            // Save -- only enabled when there are unsaved changes
            ImGui::BeginDisabled(!isDirty);
            if (ImGui::MenuItem("Save", "Ctrl+S")) {
                if (tab->document->filePath().empty()) {
                    std::string path = FileDialog::saveFile(
                        FILTER_FRAMENOTE, "framenote", "Save Framenote File");
                    if (!path.empty()) {
                        std::string err;
                        if (FileManager::save(*tab->document, path, err)) {
                            tab->document->setFilePath(path);
                            tab->document->clearDirty();
                            tab->name = path.substr(path.find_last_of("/\\") + 1);
                            m_statusMsg = "Saved!";
                            m_tabManager->recordRecentFile(path, *tab->document);
                        } else {
                            m_statusMsg = "Save failed: " + err;
                        }
                    }
                } else {
                    std::string err;
                    if (FileManager::save(*tab->document, tab->document->filePath(), err)) {
                        tab->document->clearDirty();
                        m_statusMsg = "Saved!";
                        m_tabManager->recordRecentFile(tab->document->filePath(), *tab->document);
                    }
                }
            }
            ImGui::EndDisabled();

            // Save As -- enabled whenever a tab is open
            ImGui::BeginDisabled(!hasTab);
            if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S")) {
                std::string path = FileDialog::saveFile(
                    FILTER_FRAMENOTE, "framenote", "Save As");
                if (!path.empty()) {
                    std::string err;
                    if (FileManager::save(*tab->document, path, err)) {
                        tab->document->setFilePath(path);
                        tab->document->clearDirty();
                        tab->name = path.substr(path.find_last_of("/\\") + 1);
                        m_statusMsg = "Saved!";
                        m_tabManager->recordRecentFile(path, *tab->document);
                    } else {
                        m_statusMsg = "Save failed: " + err;
                    }
                }
            }
            ImGui::EndDisabled();

            ImGui::Separator();

            // Export -- only when tab open
            ImGui::BeginDisabled(!hasTab);
            if (ImGui::BeginMenu("Export")) {
                if (ImGui::MenuItem("Export as GIF...")) {
                    m_exportType = ExportType::GIF;
                    m_showExportDialog = true;
                }
                if (ImGui::MenuItem("Export frame as PNG...")) {
                    m_exportType = ExportType::PNG;
                    m_showExportDialog = true;
                }
                if (ImGui::MenuItem("Export PNG sequence...")) {
                    m_exportType = ExportType::PNGSequence;
                    m_showExportDialog = true;
                }
                ImGui::EndMenu();
            }
            ImGui::EndDisabled();

            ImGui::Separator();
            if (ImGui::MenuItem("Quit", "Alt+F4")) {
                SDL_Event e; e.type = SDL_EVENT_QUIT;
                SDL_PushEvent(&e);
            }
            ImGui::EndMenu();
        }

        // -- Edit --------------------------------------------------------------
        if (ImGui::BeginMenu("Edit")) {
            auto* tab    = m_tabManager->activeTab();
            bool canUndo = tab && tab->history->canUndo();
            bool canRedo = tab && tab->history->canRedo();

            ImGui::BeginDisabled(!canUndo);
            if (ImGui::MenuItem("Undo", "Ctrl+Z")) {
                if (tab) {
                    HistoryEntry restored = tab->history->undo(captureCurrentEntry(tab));
                    applyHistoryEntry(tab, restored);
                }
            }
            ImGui::EndDisabled();

            ImGui::BeginDisabled(!canRedo);
            if (ImGui::MenuItem("Redo", "Ctrl+Y")) {
                if (tab) {
                    HistoryEntry restored = tab->history->redo(captureCurrentEntry(tab));
                    applyHistoryEntry(tab, restored);
                }
            }
            ImGui::EndDisabled();

            ImGui::EndMenu();
        }

        // -- Sprite ------------------------------------------------------------
        if (ImGui::BeginMenu("Sprite")) {
            auto* tab   = m_tabManager->activeTab();
            bool hasTab = tab != nullptr;

            ImGui::BeginDisabled(!hasTab);
            if (ImGui::MenuItem("Canvas Size...", "C")) {
                m_showCanvasSizeDialog = true;
                m_newCanvasW = tab->document->canvasSize().width;
                m_newCanvasH = tab->document->canvasSize().height;
            }
            ImGui::EndDisabled();
            ImGui::EndMenu();
        }

        // -- View --------------------------------------------------------------
        if (ImGui::BeginMenu("View")) {
            const char* themeLabel = Theme::current() == ThemeMode::Dark
                ? "Switch to Light Mode" : "Switch to Dark Mode";
            if (ImGui::MenuItem(themeLabel)) Theme::toggle();

            ImGui::Separator();

            auto* tab   = m_tabManager->activeTab();
            bool hasTab = tab != nullptr;

            ImGui::BeginDisabled(!hasTab);
            if (hasTab) {
                bool onion = tab->timeline->onionSkinEnabled();
                if (ImGui::MenuItem("Onion Skin", "O", &onion))
                    tab->timeline->setOnionSkin(onion);
                if (ImGui::MenuItem("Onion Skin Opacity..."))
                    m_showOnionDialog = true;
            } else {
                ImGui::MenuItem("Onion Skin", "O");
                ImGui::MenuItem("Onion Skin Opacity...");
            }
            ImGui::EndDisabled();

            ImGui::EndMenu();
        }

        // -- Help --------------------------------------------------------------
        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("About")) m_showAbout = true;
            ImGui::EndMenu();
        }

        // Status bar
        auto* tab = m_tabManager->activeTab();
        if (tab) {
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 20);
            ImGui::TextDisabled("Frame %d/%d  |  %dx%d  |  %d fps  %s  %s",
                tab->timeline->currentFrame() + 1,
                tab->document->frameCount(),
                tab->document->canvasSize().width,
                tab->document->canvasSize().height,
                tab->timeline->fps(),
                tab->document->isDirty() ? "[unsaved]" : "",
                m_statusMsg.c_str());
        } else if (!m_statusMsg.empty()) {
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 20);
            ImGui::TextDisabled("%s", m_statusMsg.c_str());
        }

        ImGui::EndMainMenuBar();
    }

    // -- About modal -----------------------------------------------------------
    if (m_showAbout) ImGui::OpenPopup("About##modal");
    if (ImGui::BeginPopupModal("About##modal", nullptr,
            ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Framenote Studio v0.2.0");
        ImGui::Text("Mascot: Nibbit :)");
        ImGui::Separator();
        ImGui::Text("Built with SDL3 + Dear ImGui");
        if (ImGui::Button("Close")) {
            m_showAbout = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void MainWindow::renderCanvasSizeDialog() {
    if (m_showCanvasSizeDialog) ImGui::OpenPopup("Canvas Size##dlg");

    if (ImGui::BeginPopupModal("Canvas Size##dlg", nullptr,
            ImGuiWindowFlags_AlwaysAutoResize)) {

        auto* tab = m_tabManager->activeTab();
        if (!tab) { ImGui::CloseCurrentPopup(); ImGui::EndPopup(); return; }

        ImGui::Text("Current: %dx%d",
            tab->document->canvasSize().width,
            tab->document->canvasSize().height);
        ImGui::Separator();
        ImGui::SetNextItemWidth(120); ImGui::InputInt("Width##cs",  &m_newCanvasW);
        ImGui::SetNextItemWidth(120); ImGui::InputInt("Height##cs", &m_newCanvasH);
        if (m_newCanvasW <    1) m_newCanvasW =    1;
        if (m_newCanvasH <    1) m_newCanvasH =    1;
        if (m_newCanvasW > 4096) m_newCanvasW = 4096;
        if (m_newCanvasH > 4096) m_newCanvasH = 4096;

        ImGui::Separator();
        ImGui::Text("Presets:");
        ImGui::SameLine(); if (ImGui::SmallButton("16"))  { m_newCanvasW=16;  m_newCanvasH=16;  }
        ImGui::SameLine(); if (ImGui::SmallButton("32"))  { m_newCanvasW=32;  m_newCanvasH=32;  }
        ImGui::SameLine(); if (ImGui::SmallButton("64"))  { m_newCanvasW=64;  m_newCanvasH=64;  }
        ImGui::SameLine(); if (ImGui::SmallButton("128")) { m_newCanvasW=128; m_newCanvasH=128; }
        ImGui::SameLine(); if (ImGui::SmallButton("256")) { m_newCanvasW=256; m_newCanvasH=256; }
        ImGui::Separator();

        if (ImGui::Button("OK", {80, 0})) {
            int oldW = tab->document->canvasSize().width;
            int oldH = tab->document->canvasSize().height;
            if (m_newCanvasW != oldW || m_newCanvasH != oldH) {
                // Snapshot all frames as a single atomic resize entry
                std::vector<Snapshot> frameSnaps;
                for (int i = 0; i < tab->document->frameCount(); ++i) {
                    auto& f = tab->document->frame(i);
                    Snapshot snap;
                    snap.frameIndex   = i;
                    snap.bufferWidth  = f.bufferWidth();
                    snap.bufferHeight = f.bufferHeight();
                    snap.pixels       = f.pixels();
                    frameSnaps.push_back(std::move(snap));
                }
                tab->history->pushResize(std::move(frameSnaps), oldW, oldH);

                for (int i = 0; i < tab->document->frameCount(); ++i) {
                    auto& frame = tab->document->frame(i);
                    frame.expandBuffer(m_newCanvasW, m_newCanvasH);
                    frame.setVisibleSize(m_newCanvasW, m_newCanvasH);
                }

                tab->document->setCanvasSize(m_newCanvasW, m_newCanvasH);
                tab->renderer->resize(m_newCanvasW, m_newCanvasH);
                tab->document->markDirty();
            }
            m_showCanvasSizeDialog = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", {80, 0})) {
            m_showCanvasSizeDialog = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void MainWindow::renderOnionOpacityDialog() {
    if (m_showOnionDialog) ImGui::OpenPopup("Onion Skin Opacity##dlg");

    if (ImGui::BeginPopupModal("Onion Skin Opacity##dlg", nullptr,
            ImGuiWindowFlags_AlwaysAutoResize)) {

        auto* tab = m_tabManager->activeTab();
        if (!tab) { ImGui::CloseCurrentPopup(); ImGui::EndPopup(); return; }

        bool onion = tab->timeline->onionSkinEnabled();
        if (ImGui::Checkbox("Enabled", &onion))
            tab->timeline->setOnionSkin(onion);

        ImGui::Spacing();
        ImGui::Text("Opacity:");
        float opacityPct = tab->timeline->onionSkinOpacity() * 100.0f;
        ImGui::SetNextItemWidth(220);
        if (ImGui::SliderFloat("##onionpct", &opacityPct, 5.0f, 100.0f, "%.0f%%",
                ImGuiSliderFlags_AlwaysClamp))
            tab->timeline->setOnionSkinOpacity(opacityPct / 100.0f);

        ImGui::Spacing();
        ImGui::Separator();
        if (ImGui::Button("Close", {80, 0})) {
            m_showOnionDialog = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    m_showOnionDialog = false;
}

// ── Drag/drop import ──────────────────────────────────────────────────────────

void MainWindow::beginDropBatch() {
    m_dropBatchPaths.clear();
    m_collectingDropBatch = true;
}

void MainWindow::addDroppedFile(const std::string& path) {
    if (path.empty())
        return;

    // Some platforms may send DROP_FILE without a DROP_BEGIN first.
    if (!m_collectingDropBatch) {
        beginDropBatch();
    }

    m_dropBatchPaths.push_back(path);
}

void MainWindow::finishDropBatch() {
    if (!m_collectingDropBatch && m_dropBatchPaths.empty())
        return;

    m_collectingDropBatch = false;
    m_dropFiles.clear();

    for (const auto& path : m_dropBatchPaths) {
        m_dropFiles.push_back(classifyDroppedFile(path));
    }

    m_dropBatchPaths.clear();

    size_t compatibleCount = compatibleDroppedFileCount();

    if (compatibleCount == 0) {
        showError("No compatible files were dropped.\n\nSupported files:\n- .framenote\n- .png");
        m_dropFiles.clear();
        return;
    }

    // Single compatible file with no mixed unsupported batch:
    // open/import directly for a fast desktop-native workflow.
    if (m_dropFiles.size() == 1 && compatibleCount == 1) {
        processDroppedFiles();
        return;
    }

    // Multiple files or mixed compatible/unsupported files:
    // show options first.
    m_dropImportMode = DropImportMode::SeparateTabs;
    m_dropOpenFramenoteFiles = true;
    m_showDropImportDialog = true;
}

MainWindow::DroppedFileInfo MainWindow::classifyDroppedFile(
    const std::string& path
) const {
    DroppedFileInfo info;
    info.path = path;
    info.name = pathFilename(path);

    std::string ext = pathExtensionLower(path);

    if (ext == ".framenote") {
        info.type = DroppedFileType::Framenote;
    }
    else if (ext == ".png") {
        info.type = DroppedFileType::Png;
    }
    else {
        info.type = DroppedFileType::Unsupported;
    }

    return info;
}

size_t MainWindow::droppedFileCount(DroppedFileType type) const {
    size_t count = 0;

    for (const auto& file : m_dropFiles) {
        if (file.type == type)
            ++count;
    }

    return count;
}

size_t MainWindow::compatibleDroppedFileCount() const {
    return droppedFileCount(DroppedFileType::Framenote) +
           droppedFileCount(DroppedFileType::Png);
}

bool MainWindow::openDroppedFramenote(const std::string& path,
                                      std::string& outError) {
    auto doc = FileManager::load(path, outError);

    if (!doc)
        return false;

    std::string name = pathFilename(path);

    m_tabManager->openDocument(std::move(doc), name, path);

    if (auto* opened = m_tabManager->activeTab()) {
        m_tabManager->recordRecentFile(path, *opened->document);
    }

    return true;
}

bool MainWindow::openDroppedPng(const std::string& path,
                                std::string& outError) {
    auto doc = FileManager::loadPngAsDocument(path, outError);

    if (!doc)
        return false;

    std::string name = pathFilename(path);

    m_tabManager->openDocument(std::move(doc), name, "");

    return true;
}

bool MainWindow::openDroppedPngSequence(const std::vector<std::string>& paths,
                                        std::string& outError) {
    auto doc = FileManager::loadPngSequenceAsDocument(
        paths,
        8,
        outError
    );

    if (!doc)
        return false;

    std::string name = "PNG Sequence";

    if (!paths.empty()) {
        std::string parent = parentFolderName(paths.front());

        if (!parent.empty())
            name = parent + " sequence";
    }

    m_tabManager->openDocument(std::move(doc), name, "");

    return true;
}

void MainWindow::processDroppedFiles() {
    int openedCount = 0;
    int ignoredCount = static_cast<int>(droppedFileCount(DroppedFileType::Unsupported));

    std::vector<std::string> errors;

    const bool usePngSequence =
        m_dropImportMode == DropImportMode::PngSequence &&
        droppedFileCount(DroppedFileType::Png) > 0;

    if (usePngSequence) {
        std::vector<std::string> pngPaths;

        for (const auto& file : m_dropFiles) {
            if (file.type == DroppedFileType::Png)
                pngPaths.push_back(file.path);
        }

        if (!pngPaths.empty()) {
            std::string err;

            if (openDroppedPngSequence(pngPaths, err)) {
                ++openedCount;
            }
            else {
                errors.push_back("PNG sequence: " + err);
            }
        }

        if (m_dropOpenFramenoteFiles) {
            for (const auto& file : m_dropFiles) {
                if (file.type != DroppedFileType::Framenote)
                    continue;

                std::string err;

                if (openDroppedFramenote(file.path, err)) {
                    ++openedCount;
                }
                else {
                    errors.push_back(file.name + ": " + err);
                }
            }
        }
    }
    else {
        for (const auto& file : m_dropFiles) {
            std::string err;
            bool opened = false;

            if (file.type == DroppedFileType::Framenote) {
                opened = openDroppedFramenote(file.path, err);
            }
            else if (file.type == DroppedFileType::Png) {
                opened = openDroppedPng(file.path, err);
            }
            else {
                continue;
            }

            if (opened) {
                ++openedCount;
            }
            else {
                errors.push_back(file.name + ": " + err);
            }
        }
    }

    if (openedCount > 0) {
        m_statusMsg = "Opened/imported " + std::to_string(openedCount) +
                      (openedCount == 1 ? " item." : " items.");

        if (ignoredCount > 0) {
            m_statusMsg += " Ignored " + std::to_string(ignoredCount) +
                           " unsupported file";
            m_statusMsg += ignoredCount == 1 ? "." : "s.";
        }
    }

    if (!errors.empty()) {
        showError("Some dropped files could not be opened:\n\n" +
                  joinImportErrors(errors));
    }

    m_dropFiles.clear();
    m_dropBatchPaths.clear();
}

void MainWindow::renderDropImportDialog() {
    if (m_showDropImportDialog) {
        ImGui::SetNextWindowPos(
            ImGui::GetMainViewport()->GetCenter(),
            ImGuiCond_Always,
            {0.5f, 0.5f}
        );

        ImGui::OpenPopup("Import Dropped Files##dropImport");
        m_showDropImportDialog = false;
    }

    ImGui::SetNextWindowPos(
        ImGui::GetMainViewport()->GetCenter(),
        ImGuiCond_Always,
        {0.5f, 0.5f}
    );

    ImGui::SetNextWindowSizeConstraints({420, 0}, {680, 560});

    if (ImGui::BeginPopupModal(
            "Import Dropped Files##dropImport",
            nullptr,
            ImGuiWindowFlags_AlwaysAutoResize)) {
        size_t pngCount        = droppedFileCount(DroppedFileType::Png);
        size_t framenoteCount  = droppedFileCount(DroppedFileType::Framenote);
        size_t unsupportedCount = droppedFileCount(DroppedFileType::Unsupported);

        ImGui::Text("Dropped files:");
        ImGui::TextDisabled(
            "%d PNG  |  %d Framenote  |  %d unsupported",
            static_cast<int>(pngCount),
            static_cast<int>(framenoteCount),
            static_cast<int>(unsupportedCount)
        );

        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::RadioButton(
                "Open each compatible file as a separate tab",
                m_dropImportMode == DropImportMode::SeparateTabs)) {
            m_dropImportMode = DropImportMode::SeparateTabs;
        }

        ImGui::BeginDisabled(pngCount == 0);

        if (ImGui::RadioButton(
                "Import PNG files as one animation sequence",
                m_dropImportMode == DropImportMode::PngSequence)) {
            m_dropImportMode = DropImportMode::PngSequence;
        }

        ImGui::EndDisabled();

        if (pngCount == 0 &&
            m_dropImportMode == DropImportMode::PngSequence) {
            m_dropImportMode = DropImportMode::SeparateTabs;
        }

        if (m_dropImportMode == DropImportMode::PngSequence &&
            framenoteCount > 0) {
            ImGui::Checkbox(
                "Also open dropped .framenote projects",
                &m_dropOpenFramenoteFiles
            );
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::Text("Files:");

        ImGui::BeginChild(
            "##dropped-file-list",
            {560, 170},
            true,
            ImGuiWindowFlags_AlwaysVerticalScrollbar
        );

        for (const auto& file : m_dropFiles) {
            const char* typeLabel = "Unsupported";

            ImVec4 color = ImVec4(1.0f, 0.45f, 0.45f, 1.0f);

            if (file.type == DroppedFileType::Png) {
                typeLabel = "PNG";
                color = ImVec4(0.75f, 0.90f, 1.0f, 1.0f);
            }
            else if (file.type == DroppedFileType::Framenote) {
                typeLabel = "Framenote";
                color = ImVec4(0.75f, 1.0f, 0.75f, 1.0f);
            }

            ImGui::TextColored(color, "[%s]", typeLabel);
            ImGui::SameLine();
            ImGui::TextUnformatted(file.name.c_str());

            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("%s", file.path.c_str());
            }
        }

        ImGui::EndChild();

        if (unsupportedCount > 0) {
            ImGui::TextDisabled(
                "Unsupported files will be ignored. Supported: .framenote, .png"
            );
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        float buttonWidth =
            (ImGui::GetContentRegionAvail().x -
             ImGui::GetStyle().ItemSpacing.x) * 0.5f;

        if (ImGui::Button("Import / Open##drop", {buttonWidth, 0})) {
            ImGui::CloseCurrentPopup();
            processDroppedFiles();
        }

        ImGui::SameLine();

        if (ImGui::Button("Cancel##drop", {buttonWidth, 0})) {
            m_dropFiles.clear();
            m_dropBatchPaths.clear();
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

} // namespace Framenote