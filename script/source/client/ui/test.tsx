
let counter = 0;

const MyComponent = () => {
    return <div key={`id_${counter}`}>
        {`Another updating div! ${counter + 1000} `}
        <div>
            This should be stable
        </div>
    </div>
}

export const test = () => {

    return <div>
        <div>
            Hello!
        </div>
        <div >
            <div>
                {`From MUD React! ${counter++} `}
            </div>
        </div>
        <div>
            <MyComponent />
        </div>
    </div>
}

