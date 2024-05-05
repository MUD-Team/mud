

let console_document: RmlElementDocument;
let console_visible = false

function on_key_up(ev: any) {
    // keycode 44 is tilde, todo make a nice table for looking up by names
    if (ev.parameters.key_identifier == 44) {
        console_visible = !console_visible
        if (console_visible) {
            console_document.Show()
            let element = console_document.GetElementById("console-input")
            element.Focus()
            mud.p_ticker_pause(true)
        }
        else {
            console_document.Hide()
            mud.p_ticker_pause(false)
        }

    }
}

function load(context: RmlContext) {
    console_document = context.LoadDocument("console.rml")
    console_document.PushToBack() // for debug ui
    context.AddEventListener('keyup', on_key_up, true)
}


export default {
    load: load
}