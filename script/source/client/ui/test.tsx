
let counter = 0;

let doit = 0;

const MyComponent = () => {
    return <div key={`id_${counter}`}>
        {`Another updating div! ${counter + 1000} `}
        <div>
            This should be stable
        </div>
    </div>
}

export const test = () => {

    const onNewGame = (hm: string) => {
        print(hm);
    }

    collectgarbage();

    doit++;

    if (doit > 100) {
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
        {(doit < 50) && <div class="button" style="margin:12dp;" click={() => onNewGame("1")} >Press Me</div>}
        <div class="button" style="margin:12dp;" click={() => onNewGame("2")} >Press Me 2</div>
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

