
import app, { AppState, IAppState } from "./app"

export const Options = () => {
    return <div>
        <div style="padding-left: 128dp; padding-top: 64dp;" ></div>
        <div class="center">
            <div class="center-menu">
                <div style="font-size: 48dp;font-weight: 800; color:#d39266;">Options</div>
                <div id="new_game" class="button" style="margin: 16dp auto;" click={() => {
                    app.state = AppState.MainMenu;
                }}>Back</div>
            </div>
        </div>
        <div>
            <tabset id="menu">
                <tab>Display</tab>
                <panel id="display" data-model="display_options">
                    <table>
                        <tr>
                            <td>
                                <div class="option">
                                    <div>Display Resolution</div>
                                </div>
                            </td>
                            <td>
                                <div class="option">
                                    <div>
                                    </div>
                                </div>
                            </td>
                            <td>
                                <div class="option">

                                </div>
                            </td>

                        </tr>
                    </table>
                </panel>
            </tabset>
        </div>
        </div>
}


        function on_key_up(ev: RmlEvent) {
    // keycode 81 is escape, todo make a nice table for looking up by names
    if (ev.parameters.key_identifier == 81) {
            ev.StopPropagation()
        app.state = AppState.MainMenu
    }
}

const createOptions = (context: RmlContext): IAppState => {

    const document = context.LoadDocument("index.rml");
        document.AddEventListener("keyup", on_key_up, true)

        return {
            document: document,
        component: Options,
        state: AppState.Options
    }
}

        export default createOptions

/*
import main_menu from "./main_menu"

let options_document: RmlElementDocument;

// Input Options

let input_datamodel: any;
let display_datamodel: any;
let sound_datamodel: any;

const default_input_options = {
    mouselook: true,
    mouselook_help: ""
}

function get_input_options() {
    const options = mud.options.get_input_options();
    input_datamodel.mouselook = options.mouselook
    input_datamodel.mouselook_help = options.mouselook_help
}

// Display Options

const default_display_options = {
    current_mode: 0,
    modes: {}
}

function get_display_options() {
    const options = mud.options.get_display_options();
    display_datamodel.current_mode = options.current_mode
    display_datamodel.modes = options.modes
}

// Sound Options

const default_sound_options = {
    music_volume: 0,
    sfx_volume: "0"
}

function get_sound_options() {
    let options = mud.options.get_sound_options();
    sound_datamodel.music_volume = options.music_volume
    sound_datamodel.sfx_volume = options.sfx_volume
}


function on_key_up(ev: RmlEvent) {
    // keycode 81 is escape, todo make a nice table for looking up by names
    if (ev.parameters.key_identifier == 81) {
        On_Options_Back()
        ev.StopPropagation()
    }
}

function load(context: RmlContext) {
    // construct data model
    input_datamodel = context.OpenDataModel("input_options", default_input_options)
    display_datamodel = context.OpenDataModel("display_options", default_display_options)
    sound_datamodel = context.OpenDataModel("sound_options", default_sound_options)

    options_document = context.LoadDocument("options.rml")
    options_document.PushToBack() // for debug ui
    options_document.Hide()

    options_document.AddEventListener("keyup", on_key_up, true)
}

function On_Options_Applied() {
    mud.options.set_input_options(input_datamodel);
    mud.options.set_display_options(display_datamodel);
    mud.options.set_sound_options(sound_datamodel);
    options_document.Hide()
    main_menu.show(true)
}

function On_Options_Back() {
    options_document.Hide()
    main_menu.show(true)
}

function On_Options_Menu() {
    get_input_options()
    get_display_options()
    get_sound_options()
    main_menu.show(false)
    options_document.Show()
}

// HACK!
(_G as any).On_Options_Applied = On_Options_Applied;
(_G as any).On_Options_Back = On_Options_Back;
(_G as any).On_Options_Menu = On_Options_Menu;

export default {
    load: load
}
*/