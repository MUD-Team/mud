local ui = require("./ui/main")

local client = mud.client
local video = mud.video

function Display()
    if client.headless or client.nodrawers then
        return
    end

    video.adjust_video_mode()
    video.start_refresh()
    ui.begin_frame()

    -- We always want to service downloads, even outside of a specific download gamestate.
    client.download_tick()

    ui.end_frame()
    video.finish_refresh()
end
--[[
function MainLoop()
    while true do
        client.run_tics()
    end
end
]]--
