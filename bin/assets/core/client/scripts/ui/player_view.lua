local player_view

local function on_blur(ev)
    mud.ui.set_relative_mouse(false)
end
local function on_focus(ev)    
    mud.ui.set_relative_mouse(true)
end

local function load(context)
    player_view = context:LoadDocument("player_view.rml")
    player_view:PushToBack() -- so plays well with debugger
    player_view:Hide()
    player_view:AddEventListener('focus', on_focus)
    player_view:AddEventListener('blur', on_blur)
end

local function on_map_loaded()
    player_view:Show()
    player_view:GetElementById("player_view"):Focus()    
end

return {
    load = load,
    on_map_loaded = on_map_loaded
}


