

import { useEffect, useState } from "../ui/react"

let counter = 0;

let doit = 0;

const UseEffectTestComponent: mud.ui.react.FC<{}> = () => {

    useEffect(() => {
        print("in");
        return () => {
            print("out");
        }
    })

    return <div></div>
}

const MyComponent: mud.ui.react.FC<{ which: string }> = ({ which }) => {

    const [value, setState] = useState({ v1: which == "2", v2: which != "2" });
    // limit to one state per component?
    //const [ovalue, setOState] = useState(false);

    setState({ v1: !value.v1, v2: !value.v2 });

    return <div>
        {value.v1 && <div class="button" click={() => print(`${which} - ${value.v1} - ${value.v2}`)}>
            {`Another updating div! ${counter}`}
        </div>}
        {value.v2 &&
            <div class="button" click={() => print(`${which} - ${value.v1} - ${value.v2}`)}>
                This should be stable
            </div>}
    </div>
}

const WhatAComponent = () => {

    const onNewGame = (hm: string) => {
        print(hm);
    }

    collectgarbage();

    doit++;

    if (doit > 1000) {
        doit = 0;
    }

    /*
    return <div>Hello
        {doit < 50 &&<div class="button" click={() => onNewGame("1 " + doit)}>
            World
        </div>}
    </div>;
    */


    return <div>
        <div>
            {`From MUD React! ${counter++} `}
        </div>
        <div>
            {(doit < 500) && <UseEffectTestComponent key="unique_key" />}
            <MyComponent which="1" key="my_friggin_key_1" />
            <MyComponent which="2" key="my_friggin_key_2" />
            <MyComponent which="3" key="my_friggin_key_3" />
        </div>
        {(doit < 500) && <div key="button1" class="button" style="margin:12dp;" click={() => onNewGame("1")} >Press Me</div>}
        <div>
            {(doit < 500) && <div><div key="button1" class="button" style="margin:12dp;" click={() => onNewGame("3")} >Press Me 3</div></div>}
            <div key="button2" class="button" style="margin:12dp;" click={() => onNewGame("4")} >Press Me 4</div>
        </div>
        <div key="button2" class="button" style="margin:12dp;" click={() => onNewGame("2")} >Press Me 2</div>

    </div>

}

export const test = () => {

    function whatatest() {
        return <WhatAComponent/>
    }

    return <div>
        {whatatest()}
    </div>
}

/*

        <div >
            <div>
                {`From MUD React! ${counter++} `}
            </div>
            <div key={`__counter${counter}`} class="button" style="margin:12dp;" click={onNewGame} >Press Me</div>
        </div>
        <div>
            <MyComponent />
            <MyComponent />
            <MyComponent />
        </div>
*/

