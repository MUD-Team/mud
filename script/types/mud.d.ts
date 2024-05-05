
declare type UIModule = {
    set_relative_mouse(this: void, relative: boolean): void;
    begin_frame(this: void): void;
    end_frame(this: void): void;
}

declare type ClientModule = {
    run_tics(this: void): void;
    download_tick(this: void): void;
    headless: boolean;
    nodrawers: boolean;
}

declare type VideoModule = {
    width: number;
    height: number;
    initialized: boolean;
    start_refresh(this: void): void;
    finish_refresh(this: void): void;
    adjust_video_mode(this: void): void;
}

declare type OptionsModule = {
    get_input_options(this: void): any;
    set_input_options(this: void, options: any): void;

    get_display_options(this: void): any;
    set_display_options(this: void, options: any): void;

    get_sound_options(this: void): any;
    set_sound_options(this: void, options: any): void;
}

declare const mud: {
    add_command: (this: void, command: string) => void;
    p_ticker_pause: (this: void, pause: boolean) => void;
    ui: UIModule;
    client: ClientModule;
    video: VideoModule;
    options: OptionsModule;
}

// RmlUi
declare const Vector2i: any;

declare type RmlEvent = {
    StopPropagation: () => void;

    parameters: any;
}

declare type RmlElement = {
    AddEventListener: (event: string, handler: (ev?: any) => void, inCapturePhase: boolean) => void;
    GetElementById: (id: string) => RmlElement;
    Hide: () => void;
    Show: () => void;
    Focus: () => void;
}

declare type RmlElementDocument = RmlElement & {
    PushToBack: () => void;
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
