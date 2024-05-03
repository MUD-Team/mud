local player_view

local function load(context)
    player_view = context:LoadDocument("player_view.rml")
    player_view:PushToBack() -- so plays well with debugger
    player_view:Hide()
end

local function on_map_loaded()
    player_view:Show()
    player_view:Focus()
end

return {
    load = load,
    on_map_loaded = on_map_loaded
}


