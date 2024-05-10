import app, { AppState, IAppState } from "./app"

export const MainMenu = () => {
    return <div>
        <div style="padding-left: 128dp; padding-top: 64dp;" ></div>
        <div class="center">
            <div class="center-menu">
                <div style="font-size: 48dp;font-weight: 800; color:#d39266;">MUD</div>
                <div id="new_game" class="button" style="margin: 16dp auto;" click={() => {
                }}>New Game</div>

                <div id="quit" class="button" style="margin: 16dp auto;" click={() => { 
                    app.state = AppState.Options;
                }}>Options</div>

                <div id="quit" class="button" style="margin: 16dp auto;" click={() => mud.add_command('quit')}>Quit</div>
            </div>
        </div>
    </div>
}

const createMainMenu = (context: RmlContext): IAppState => {

    return {
        document: context.LoadDocument("index.rml"),
        component: MainMenu,
        state: AppState.MainMenu
    }
}

export default createMainMenu



/*
import { render } from "./react";
import app from "./app"

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

function myrender()
{
    render(menu_document, menu_document.GetElementById("react-root"), app);
}

export default {
    show: show,
    load: load,
    on_map_loaded: on_map_loaded,
    render: myrender
}
*/