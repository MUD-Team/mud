

local console_document
local console_visible = false

local function on_key_up(ev)
    -- keycode 44 is tilde, todo make a nice table for looking up by names
    if (ev.parameters.key_identifier == 44) then
        console_visible = not console_visible
        if(console_visible) then
            console_document:Show()
            local element = console_document:GetElementById("console-input")            
            element:Focus()
            mud.p_ticker_pause(true)
        else 
            console_document:Hide()
            mud.p_ticker_pause(false)
        end
    end
end

local function load(context)
    console_document = context:LoadDocument("console.rml")
    console_document:PushToBack() -- for debug ui
    context:AddEventListener('keyup', on_key_up, true)
end

return {
    load = load
}
