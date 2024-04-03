local menu_document;
local player_view;
local menu_context;
local menu_shown

local function show(shown)
    menu_shown = shown
    if shown then
        menu_document:Show()
        menu_document:Focus()
    else
        menu_document:Hide()
        if player_view then
            player_view:Focus()
        end
    end

    if (menu_shown) then
        mud.p_ticker_pause(true)
    else
        mud.p_ticker_pause(false)
    end
end

local function on_key_up(ev)
    if not player_view then return end
    -- keycode 81 is escape, todo make a nice table for looking up by names
    if (ev.parameters.key_identifier == 81) then
        show(not menu_shown)
    end
end

local function load(context)
    menu_context = context
    menu_document = context:LoadDocument("main_menu.rml")
    menu_document:PushToBack() -- so plays well with debugger
    show(true)

    context:AddEventListener('keyup', on_key_up, true)
end

-- global for ui access, todo fix this
function On_New_Game()
    mud.add_command('map map01')
    show(false)
    -- todo, should not be recycling view here
    if not player_view then
        player_view = menu_context:LoadDocument("player_view.rml")
        player_view:PushToBack() -- so plays well with debugger
        player_view:Show();
        player_view:Focus();
    end
end

return {
    show = show,
    load = load
}
