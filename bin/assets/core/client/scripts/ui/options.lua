local main_menu = require("./main_menu")

local options_document

-- Input Options

local input_datamodel
local display_datamodel
local sound_datamodel

local default_input_options = {
    mouselook = true,
    mouselook_help = ""
}

local function get_input_options()
    local options = mud.options.get_input_options();
    input_datamodel.mouselook = options.mouselook
    input_datamodel.mouselook_help = options.mouselook_help
end

-- Display Options

local default_display_options = {
    current_mode = 0,
    modes = {}
}

local function get_display_options()
    local options = mud.options.get_display_options();
    display_datamodel.current_mode = options.current_mode
    display_datamodel.modes = options.modes
end

-- Sound Options
local default_sound_options = {
    music_volume = 0,
    sfx_volume = "0"
}

local function get_sound_options()
    local options = mud.options.get_sound_options();
    sound_datamodel.music_volume = options.music_volume
    sound_datamodel.sfx_volume = options.sfx_volume
end


local function on_key_up(ev)
    -- keycode 81 is escape, todo make a nice table for looking up by names
    if (ev.parameters.key_identifier == 81) then
        On_Options_Back()
        ev:StopPropagation()
    end
end

local function load(context)
    -- construct data model
    input_datamodel = context:OpenDataModel("input_options", default_input_options)
    display_datamodel = context:OpenDataModel("display_options", default_display_options)
    sound_datamodel = context:OpenDataModel("sound_options", default_sound_options)

    options_document = context:LoadDocument("options.rml")
    options_document:PushToBack() -- for debug ui
    options_document:Hide()

    options_document:AddEventListener("keyup", on_key_up, true)
end

function On_Options_Applied()
    mud.options.set_input_options(input_datamodel);
    mud.options.set_display_options(display_datamodel);
    mud.options.set_sound_options(sound_datamodel);
    options_document:Hide()
    main_menu.show(true)
end

function On_Options_Back()
    options_document:Hide()
    main_menu.show(true)
end

function On_Options_Menu()
    get_input_options()
    get_display_options()
    get_sound_options()
    main_menu.show(false)
    options_document:Show()
end

return {
    load = load
}
