
let counter = 0;

export const test = () => {

    return <div key={`stable1`}>
        <div >
            <div key={`key_${counter++}`}>
                {`From MUD React! ${counter} `}
            </div>
        </div>
        <div key="stable3">
            Hello!
        </div>

    </div>
}

