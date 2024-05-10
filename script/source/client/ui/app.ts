
import { render } from "./react"

export enum AppState {
    None = "None",
    MainMenu = "MainMenu",
    Options = "Options",
    Play = "Play"
}

export interface IAppState {
    document: RmlElementDocument;
    component: any;
    state: AppState
}

class App {

    registerState(state: IAppState) {
        this.states.set(state.state, state);
    }

    render() {
        const state = this.states.get(this._state);
        if (!state) {
            return;
        }
        state.document.Show();
        state.document.Focus();

        render(state.document, state.document.GetElementById("react-root"), state.component)
    }

    get context(): RmlContext {
        assert(this._context);
        return this._context!;
    }

    set context(context: RmlContext) {
        assert(!this._context);
        this._context = context;
    }


    get state() {
        return this._state;
    }

    set state(nstate: AppState) {

        if (this._state == nstate) {
            return;
        }

        this.states.forEach(s => {
            s.document.Hide();
        })

        const state = this.states.get(nstate);
        state?.document.Show()
        state?.document.Focus()

        this._state = nstate;
    }

    private _state = AppState.None;
    private _context?: RmlContext;

    private states = new Map<AppState, IAppState>();

}

export default new App();
