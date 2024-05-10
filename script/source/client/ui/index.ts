
import console from "./console"
import "./react"
import app, { AppState } from "./app"
import createMainMenu from "./main_menu"
import createOptions from "./options"

const video = mud.video
const ui = mud.ui

const play_context = rmlui.CreateContext("play", Vector2i.new(video.width, video.height))
app.context = play_context;

console.load(play_context)

createMainMenu();
createOptions();

app.state = AppState.Options;

function begin_frame() {     

    collectgarbage()
    app.render()
    ui.begin_frame()
    play_context.Update()
}

function end_frame() {
    play_context.Render()
    ui.end_frame()
}

function On_Map_Loaded() {
    //main_menu.on_map_loaded()
    //player_view.on_map_loaded()
}

// HACK!
(_G as any).On_Map_Loaded = On_Map_Loaded;

// @noSelf
export default {
    begin_frame: begin_frame,
    end_frame: end_frame
}

