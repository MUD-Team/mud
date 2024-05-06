
import {test} from "./test"

let menu_document: RmlElementDocument;
let menu_shown: boolean;

function show(shown: boolean) {
    menu_shown = shown
    if (shown) {
        menu_document.Show()
        menu_document.Focus()
    }
    else {
        menu_document.Hide()
    }

    if (menu_shown) {
        mud.p_ticker_pause(true)
    } else {
        mud.p_ticker_pause(false)
    }
}

function render()
{
    mud.ui.react.render(menu_document, menu_document.GetElementById("react-root"), test);
}

function on_key_up(ev: any) {
    // keycode 81 is escape, todo make a nice table for looking up by names
    if (ev.parameters.key_identifier == 81) {
        show(!menu_shown)
    }
}

function load(context: RmlContext) {
    menu_document = context.LoadDocument("main_menu.rml");
    menu_document.PushToBack(); // so plays well with debugger
    show(true);
    context.AddEventListener('keyup', on_key_up, true)
}

function on_map_loaded() {
    show(false)
}

// global for ui access, todo fix this
function On_New_Game() {
    mud.add_command('map map01')
    show(false)
}

(_G as any).On_New_Game = On_New_Game;

export default {
    show: show,
    load: load,
    on_map_loaded: on_map_loaded,
    render: render
}