
import ui from "./ui"

const client = mud.client
const video = mud.video

const display = () => {
    if (client.headless || client.nodrawers) {
        return
    }

    video.adjust_video_mode()
    video.start_refresh()
    ui.begin_frame()

    // We always want to service downloads, even outside of a specific download gamestate.
    client.download_tick()

    ui.end_frame()
    video.finish_refresh()
}


function mainLoop() {
    while (true) {
        client.run_tics()
    }
}

// HACK!
(_G as any).MainLoop = mainLoop;
(_G as any).Display = display;


export default {
    display: display,
    mainLoop: mainLoop
}