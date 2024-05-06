
import console from "./console"
import main_menu from "./main_menu"
import player_view from "./player_view"
import options from "./options"

const video = mud.video
const ui = mud.ui

const play_context = rmlui.CreateContext("play", Vector2i.new(video.width, video.height))

console.load(play_context)
main_menu.load(play_context)
options.load(play_context)
player_view.load(play_context)

function begin_frame() {
    main_menu.render()
    ui.begin_frame()
    play_context.Update()
}

function end_frame() {
    play_context.Render()
    ui.end_frame()
}

function On_Map_Loaded() {
    main_menu.on_map_loaded()
    player_view.on_map_loaded()
}

// HACK!
(_G as any).On_Map_Loaded = On_Map_Loaded;

// @noSelf
export default {
    begin_frame: begin_frame,
    end_frame: end_frame
}

