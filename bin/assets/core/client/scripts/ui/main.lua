-- instantiate

local console = require("./console")
local main_menu = require("./main_menu")
local player_view = require("./player_view")

local video = mud.video
local ui = mud.ui

local play_context = rmlui:CreateContext("play", Vector2i.new(video.width, video.height))

console.load(play_context)
main_menu.load(play_context)
player_view.load(play_context)

local begin_frame = function()
    ui.begin_frame()
    play_context:Update()
end

local end_frame = function()
    play_context:Render()
    ui.end_frame()
end

function On_Map_Loaded()
    main_menu.on_map_loaded()
    player_view.on_map_loaded()
end

return {
    begin_frame = begin_frame,
    end_frame = end_frame
}


