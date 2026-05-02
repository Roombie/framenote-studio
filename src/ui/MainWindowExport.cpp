#include "ui/MainWindow.h"
#include "ui/ModalUtils.h"

#include "io/FileDialog.h"
#include "io/GifExporter.h"
#include "io/PngExporter.h"
#include "io/ExportOptions.h"

#include <imgui.h>

#include <algorithm>
#include <string>

namespace Framenote {

namespace {

constexpr const char* FILTER_PNG =
    "PNG Image (*.png)\0*.png\0All Files (*.*)\0*.*\0";

constexpr const char* FILTER_GIF =
    "GIF Image (*.gif)\0*.gif\0All Files (*.*)\0*.*\0";

} // namespace

void MainWindow::renderExportDialog() {
    if (m_showExportDialog) {
        auto* tab = m_tabManager->activeTab();

        if (tab) {
            int frameCount = tab->document->frameCount();

            m_exportFps = tab->timeline->fps();
            m_exportLoop = tab->timeline->looping();

            m_exportUseFrameRange = false;
            m_exportStartFrame = 1;
            m_exportEndFrame = std::max(1, frameCount);
            m_exportPngFrame = tab->timeline->currentFrame() + 1;
        }

        ImGui::OpenPopup("Export##settingsDlg");
        m_showExportDialog = false;
    }

    ModalUtils::centerNextWindowOnAppearing();

    ImGui::SetNextWindowSizeConstraints({380, 0}, {540, 700});

    if (ImGui::BeginPopupModal(
            "Export##settingsDlg",
            nullptr,
            ImGuiWindowFlags_AlwaysAutoResize)) {

        ModalUtils::keepCurrentWindowInsideMainViewport();

        auto* tab = m_tabManager->activeTab();

        if (!tab) {
            ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
            return;
        }

        int cw = tab->document->canvasSize().width;
        int ch = tab->document->canvasSize().height;
        int frameCount = tab->document->frameCount();

        m_exportScale = std::clamp(m_exportScale, 1, 64);
        m_exportFps = std::clamp(m_exportFps, 1, 60);
        m_exportStartFrame = std::clamp(m_exportStartFrame, 1, std::max(1, frameCount));
        m_exportEndFrame = std::clamp(m_exportEndFrame, 1, std::max(1, frameCount));
        m_exportPngFrame = std::clamp(m_exportPngFrame, 1, std::max(1, frameCount));
        m_exportBackground = std::clamp(m_exportBackground, 0, 2);

        if (m_exportStartFrame > m_exportEndFrame)
            m_exportEndFrame = m_exportStartFrame;

        const char* typeName =
            m_exportType == ExportType::GIF
                ? "Export as GIF"
                : m_exportType == ExportType::PNG
                    ? "Export Frame as PNG"
                    : "Export PNG Sequence";

        ImGui::TextUnformatted(typeName);
        ImGui::Separator();
        ImGui::Spacing();

        // Scale
        ImGui::Text("Scale:");
        ImGui::SameLine();

        if (ImGui::RadioButton("1x##sc", m_exportScale == 1))
            m_exportScale = 1;

        ImGui::SameLine();

        if (ImGui::RadioButton("2x##sc", m_exportScale == 2))
            m_exportScale = 2;

        ImGui::SameLine();

        if (ImGui::RadioButton("4x##sc", m_exportScale == 4))
            m_exportScale = 4;

        ImGui::SameLine();

        if (ImGui::RadioButton("8x##sc", m_exportScale == 8))
            m_exportScale = 8;

        ImGui::TextDisabled(
            "Output size: %d x %d px",
            cw * m_exportScale,
            ch * m_exportScale
        );

        ImGui::Spacing();

        // Background
        const char* backgroundItems[] = {
            "Transparent",
            "White",
            "Black"
        };

        ImGui::SetNextItemWidth(180);

        ImGui::Combo(
            "Background##export",
            &m_exportBackground,
            backgroundItems,
            IM_ARRAYSIZE(backgroundItems)
        );

        ImGui::Spacing();

        // Frame options
        if (m_exportType == ExportType::PNG) {
            ImGui::SetNextItemWidth(120);
            ImGui::InputInt("Frame##singlePngFrame", &m_exportPngFrame);

            m_exportPngFrame = std::clamp(
                m_exportPngFrame,
                1,
                std::max(1, frameCount)
            );

            ImGui::TextDisabled(
                "Exporting frame %d of %d",
                m_exportPngFrame,
                frameCount
            );

            ImGui::Spacing();
        }
        else {
            ImGui::Checkbox("Custom frame range##export", &m_exportUseFrameRange);

            ImGui::BeginDisabled(!m_exportUseFrameRange);

            ImGui::SetNextItemWidth(120);
            ImGui::InputInt("Start frame##exportRangeStart", &m_exportStartFrame);

            ImGui::SetNextItemWidth(120);
            ImGui::InputInt("End frame##exportRangeEnd", &m_exportEndFrame);

            ImGui::EndDisabled();

            m_exportStartFrame = std::clamp(
                m_exportStartFrame,
                1,
                std::max(1, frameCount)
            );

            m_exportEndFrame = std::clamp(
                m_exportEndFrame,
                1,
                std::max(1, frameCount)
            );

            if (m_exportStartFrame > m_exportEndFrame)
                m_exportEndFrame = m_exportStartFrame;

            int exportedFrames = m_exportUseFrameRange
                ? (m_exportEndFrame - m_exportStartFrame + 1)
                : frameCount;

            if (m_exportUseFrameRange) {
                ImGui::TextDisabled(
                    "Exporting frames %d-%d (%d frame%s)",
                    m_exportStartFrame,
                    m_exportEndFrame,
                    exportedFrames,
                    exportedFrames == 1 ? "" : "s"
                );
            }
            else {
                ImGui::TextDisabled(
                    "Exporting all %d frame%s",
                    frameCount,
                    frameCount == 1 ? "" : "s"
                );
            }

            ImGui::Spacing();
        }

        // GIF-specific
        if (m_exportType == ExportType::GIF) {
            ImGui::SetNextItemWidth(120);
            ImGui::SliderInt("FPS##exp", &m_exportFps, 1, 60);

            m_exportFps = std::clamp(m_exportFps, 1, 60);

            ImGui::SameLine();
            ImGui::TextDisabled("(doc: %d)", tab->timeline->fps());

            ImGui::Checkbox("Loop##exp", &m_exportLoop);

            int exportedFrames = m_exportUseFrameRange
                ? (m_exportEndFrame - m_exportStartFrame + 1)
                : frameCount;

            float duration = exportedFrames / static_cast<float>(m_exportFps);

            ImGui::TextDisabled(
                "%d frame%s  -  %.2f sec",
                exportedFrames,
                exportedFrames == 1 ? "" : "s",
                duration
            );

            ImGui::Spacing();
        }

        if (m_exportType == ExportType::PNGSequence) {
            int exportedFrames = m_exportUseFrameRange
                ? (m_exportEndFrame - m_exportStartFrame + 1)
                : frameCount;

            ImGui::TextDisabled(
                "Numbered output: base_0001.png, base_0002.png..."
            );

            if (m_exportUseFrameRange) {
                ImGui::TextDisabled(
                    "Actual numbering uses source frame numbers."
                );
            }

            ImGui::TextDisabled(
                "%d PNG file%s will be written.",
                exportedFrames,
                exportedFrames == 1 ? "" : "s"
            );

            ImGui::Spacing();
        }

        ImGui::Separator();
        ImGui::Spacing();

        float btnW = (
            ImGui::GetContentRegionAvail().x -
            ImGui::GetStyle().ItemSpacing.x
        ) / 2.0f;

        if (ImGui::Button("Export##go", {btnW, 0})) {
            ImGui::CloseCurrentPopup();

            std::string err;

            ExportBackgroundMode background =
                static_cast<ExportBackgroundMode>(m_exportBackground);

            if (m_exportType == ExportType::GIF) {
                std::string path = FileDialog::saveFile(
                    FILTER_GIF,
                    "gif",
                    "Export as GIF"
                );

                if (!path.empty()) {
                    GifExporter::Options opts;
                    opts.fps = m_exportFps;
                    opts.loop = m_exportLoop;
                    opts.scale = m_exportScale;
                    opts.background = background;
                    opts.useFrameRange = m_exportUseFrameRange;
                    opts.startFrame = m_exportStartFrame - 1;
                    opts.endFrame = m_exportEndFrame - 1;

                    if (GifExporter::exportGif(*tab->document, path, opts, err)) {
                        m_statusMsg = "GIF exported!";
                    }
                    else {
                        showError("Export failed:\n" + err);
                    }
                }
            }
            else if (m_exportType == ExportType::PNG) {
                std::string path = FileDialog::saveFile(
                    FILTER_PNG,
                    "png",
                    "Export Frame as PNG"
                );

                if (!path.empty()) {
                    ExportRasterOptions opts;
                    opts.scale = m_exportScale;
                    opts.background = background;

                    int frameIndex = m_exportPngFrame - 1;

                    if (PngExporter::exportFrame(
                            *tab->document,
                            frameIndex,
                            path,
                            err,
                            opts)) {
                        m_statusMsg = "PNG exported!";
                    }
                    else {
                        showError("Export failed:\n" + err);
                    }
                }
            }
            else if (m_exportType == ExportType::PNGSequence) {
                std::string path = FileDialog::saveFile(
                    FILTER_PNG,
                    "png",
                    "Export PNG Sequence (choose base name)"
                );

                if (!path.empty()) {
                    std::string base = path;

                    if (base.size() > 4 && base.substr(base.size() - 4) == ".png")
                        base = base.substr(0, base.size() - 4);

                    ExportRasterOptions opts;
                    opts.scale = m_exportScale;
                    opts.background = background;
                    opts.useFrameRange = m_exportUseFrameRange;
                    opts.startFrame = m_exportStartFrame - 1;
                    opts.endFrame = m_exportEndFrame - 1;

                    if (PngExporter::exportSequence(
                            *tab->document,
                            base,
                            err,
                            opts)) {
                        m_statusMsg = "PNG sequence exported!";
                    }
                    else {
                        showError("Export failed:\n" + err);
                    }
                }
            }
        }

        ImGui::SameLine();

        if (ImGui::Button("Cancel##exp", {btnW, 0}))
            ImGui::CloseCurrentPopup();

        ImGui::EndPopup();
    }
}

} // namespace Framenote