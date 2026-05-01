#include "core/DocumentTab.h"
#include "core/Selection.h"

namespace Framenote {

std::unique_ptr<DocumentTab> DocumentTab::createBlank(
    SDL_Renderer* sdlRenderer,
    const std::string& name,
    int w, int h, int fps)
{
    auto tab = std::make_unique<DocumentTab>();
    tab->name     = name;
    tab->document = std::make_unique<Document>(w, h);
    tab->document->setFps(fps);
    tab->document->addFrame();
    tab->timeline = std::make_unique<Timeline>();
    tab->timeline->setFrameCount(1);
    tab->timeline->setFps(fps);
    tab->history   = std::make_unique<History>();
    tab->renderer  = std::make_unique<CanvasRenderer>(sdlRenderer, w, h);
    tab->selection = std::make_unique<Selection>(w, h);
    return tab;
}

} // namespace Framenote