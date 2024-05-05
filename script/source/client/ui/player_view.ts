let player_view: RmlElementDocument

function on_blur(ev: RmlElementDocument) {
    mud.ui.set_relative_mouse(false)
}

function on_focus(ev: RmlElementDocument) {
    mud.ui.set_relative_mouse(true)
}

function load(context: RmlContext) {
    player_view = context.LoadDocument("player_view.rml")
    player_view.PushToBack() // so plays well with debugger
    player_view.Hide()
    player_view.AddEventListener('focus', on_focus, true)
    player_view.AddEventListener('blur', on_blur, true)
}

function on_map_loaded() {
    player_view.Show()
    player_view.GetElementById("player_view").Focus()
}

export default {
    load: load,
    on_map_loaded: on_map_loaded
}