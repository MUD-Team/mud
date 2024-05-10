

// useState hook

type State = {
    initialValue: any;
    value: any;
    setter: (this: void, newValue: any) => void;
}

const componentStates = new Map<string, State>();

export const useState = function (this: void, initialValue: any): [any, (newValue: any) => void] {

    const function_key = (_G as any).__mud_react_component_key as string;

    assert(function_key);

    let state = componentStates.get(function_key);
    if (!state) {
        state = {
            initialValue: initialValue,
            value: initialValue,
            setter: (newValue: any) => {
                const s = componentStates.get(function_key);
                assert(s);
                if (s) {
                    if (s.value !== newValue) {
                        //const render_key = (_G as any).__mud_react_render_key as number;
                        s.value = newValue;
                        //((_G as any).__mud_react_dirty_render_keys as any)[render_key] = true;
                    }
                }
            }
        }
        componentStates.set(function_key, state);
    }

    return [state.value, state.setter];

}

// useEffect hook

type EffectKey = string;

type Effect = {
    key: EffectKey;
    setup: mud.ui.react.EffectCallback;
    cleanup?: () => void,
    currentdeps?: mud.ui.react.DependencyList;
    olddeps?: mud.ui.react.DependencyList;
}

const componentEffects = new Map<string, Effect>();

let frameEffects = new Set<EffectKey>;
let lastFrameEffects = new Set<EffectKey>();

export function useEffect(this: void, effect: mud.ui.react.EffectCallback, deps?: mud.ui.react.DependencyList): void {
    const effect_key = (_G as any).__mud_react_component_key as EffectKey;
    assert(effect_key);

    const ceffect = componentEffects.get(effect_key);
    if (!ceffect) {

        const neffect = {
            key: effect_key,
            setup: effect,
            deps: deps
        }
        frameEffects.add(neffect.key);
        componentEffects.set(effect_key, neffect);
        return;
    }

    frameEffects.add(ceffect.key);
}

export function render(this: void, document: RmlElementDocument, parent: RmlElement, root: Function): void {

    mud.ui.react.render(document, parent, root);

    // run setup
    frameEffects.forEach(key => {

        if (!lastFrameEffects.has(key)) {
            const effect = componentEffects.get(key)!;
            effect.cleanup = componentEffects.get(key)!.setup();
        }     
    });

    // run setup
    lastFrameEffects.forEach(key => {

        // run cleanup
        if (!frameEffects.has(key)) {
            const effect = componentEffects.get(key)!;
            if (effect.cleanup) {
                effect.cleanup();                
            }
            componentEffects.delete(key);
        } 
    
    });

    lastFrameEffects = frameEffects;
    frameEffects = new Set<EffectKey>();
}