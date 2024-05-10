

// Trigger debugger
declare function dbg(): void;

declare namespace mud {
    export function add_command(this: void, command: string): void;
    export function p_ticker_pause(this: void, pause: boolean): void;
}

declare namespace mud.client {
    export function run_tics(this: void): void;
    export function download_tick(this: void): void;
    export const headless: boolean;
    export const nodrawers: boolean;
}

declare namespace mud.video {
    export const width: number;
    export const height: number;
    export const initialized: boolean;
    export function start_refresh(this: void): void;
    export function finish_refresh(this: void): void;
    export function adjust_video_mode(this: void): void;
}

declare namespace mud.options {
    export function get_input_options(this: void): any;
    export function set_input_options(this: void, options: any): void;

    export function get_display_options(this: void): any;
    export function set_display_options(this: void, options: any): void;

    export function get_sound_options(this: void): any;
    export function set_sound_options(this: void, options: any): void;
}

declare namespace mud.ui {
    export function set_relative_mouse(this: void, relative: boolean): void;
    export function begin_frame(this: void): void;
    export function end_frame(this: void): void;
}



declare namespace mud.ui.react {

    type EffectCallback = () => any;
    type DependencyList = readonly unknown[];

    interface FunctionComponent<P = {}> {
        (
            props: P & { key: string },
        ): any // return type
    }

    type FC<P = {}> = FunctionComponent<P>;

    export function createElement(this: void, type: string | Function, props?: any, ...children: any[]): any;
    export function render(this: void, document: RmlElementDocument, parent: RmlElement, root: Function): void;
    //export function useState(this: void, initialValue: any): [any, (value: any) => void];
}



// RmlUi
declare const Vector2i: any;

declare type RmlEvent = {
    StopPropagation: () => void;

    parameters: any;
}

declare type RmlElement = {
    GetAttribute(name: string): any;
    SetAttribute(name: string, value: any): void;
    AddEventListener: (event: string, handler: (ev?: any) => void, inCapturePhase: boolean) => void;
    GetElementById: (id: string) => RmlElement;
    AppendChild(child: RmlElement): RmlElement | undefined;
    RemoveChild(child: RmlElement): boolean;
    ReplaceChild(insert: RmlElement, replaced: RmlElement): RmlElement;
    Hide: () => void;
    Show: () => void;
    Focus: () => void;
    id: string;
    tag_name: string;
    parent_node?: RmlElement;

    first_child?: RmlElement;
    next_sibling?: RmlElement;
    previous_sibling?: RmlElement;
    last_child?: RmlElement;
    inner_rml?: string;
}

declare type RmlElementDocument = RmlElement & {
    PushToBack: () => void;
    CreateElement(tag: string): RmlElement;
    CreateTextNode(text: string): RmlElement;
}

declare type RmlContext = {
    AddEventListener: (event: string, handler: (ev?: RmlEvent) => void, inCapturePhase: boolean) => void;
    Update: () => void;
    Render: () => void;
    LoadDocument: (name: string) => RmlElementDocument;
    OpenDataModel: (name: string, dataModel: any) => any;
}

declare const rmlui: {
    CreateContext: (name: string, dimensions: any /*Vector2i*/) => RmlContext;
}


// MUD React

declare namespace JSX {
    interface IntrinsicElements {
        div: any,
        tabset: any,
        tab: any,
        panel: any,
        table:any,
        tr:any,
        td:any,
        select:any,
        option:any
    }
}