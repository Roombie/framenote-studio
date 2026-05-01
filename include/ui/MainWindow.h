#pragma once

#include "ui/TabManager.h"
#include "tools/ToolManager.h"

#include <string>
#include <vector>

namespace Framenote {

class MainWindow {
public:
    MainWindow(TabManager* tabManager, ToolManager* toolManager);

    void render();

    // SDL drag/drop entry points.
    void beginDropBatch();
    void addDroppedFile(const std::string& path);
    void finishDropBatch();

private:
    void renderMenuBar();
    void renderCanvasSizeDialog();
    void renderExportDialog();
    void renderOnionOpacityDialog();
    void renderErrorDialog();
    void renderDropImportDialog();

    void showError(const std::string& msg);
    bool doSave(DocumentTab* tab, const std::string& path);

    enum class ExportType {
        GIF,
        PNG,
        PNGSequence
    };

    enum class DroppedFileType {
        Framenote,
        Png,
        Unsupported
    };

    enum class DropImportMode {
        SeparateTabs,
        PngSequence
    };

    struct DroppedFileInfo {
        std::string path;
        std::string name;
        DroppedFileType type = DroppedFileType::Unsupported;
    };

    DroppedFileInfo classifyDroppedFile(const std::string& path) const;

    size_t droppedFileCount(DroppedFileType type) const;
    size_t compatibleDroppedFileCount() const;

    void processDroppedFiles();

    bool openDroppedFramenote(const std::string& path, std::string& outError);
    bool openDroppedPng(const std::string& path, std::string& outError);
    bool openDroppedPngSequence(const std::vector<std::string>& paths,
                                std::string& outError);

private:
    TabManager*  m_tabManager  = nullptr;
    ToolManager* m_toolManager = nullptr;

    bool        m_showAbout            = false;
    bool        m_showOnionDialog      = false;
    bool        m_showCanvasSizeDialog = false;
    bool        m_showExportDialog     = false;
    bool        m_showErrorDialog      = false;
    ExportType  m_exportType           = ExportType::GIF;

    int         m_newCanvasW           = 128;
    int         m_newCanvasH           = 128;

    // Export settings
    int         m_exportFps            = 8;
    bool        m_exportLoop           = true;
    int         m_exportScale          = 1;

    // Drag/drop import state
    bool                         m_collectingDropBatch    = false;
    bool                         m_showDropImportDialog   = false;
    bool                         m_dropOpenFramenoteFiles = true;
    DropImportMode               m_dropImportMode         = DropImportMode::SeparateTabs;
    std::vector<std::string>     m_dropBatchPaths;
    std::vector<DroppedFileInfo> m_dropFiles;

    std::string m_statusMsg;
    std::string m_errorMsg;
};

} // namespace Framenote