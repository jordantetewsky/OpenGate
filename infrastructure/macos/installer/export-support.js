// SPDX-License-Identifier: MIT
// JXA uses stock macOS Foundation; no Python or additional runtime is required.
ObjC.import('Foundation');
function run(argv) {
    const home=argv[0], destination=argv[1];
    function read(path) {
        const value=$.NSString.stringWithContentsOfFileEncodingError(path,$.NSUTF8StringEncoding,null);
        return ObjC.unwrap(value) || '';
    }
    const allowed=['time','event','component','vendor','module','build','session','event_sequence','operation','success','ready','reason','phase','result','duration_ms','snapshot_ms','persist_ms','menu_ms','header_ms'];
    const records=[];
    // Preserve rollback evidence: read old logs without moving or deleting them.
    for(const directory of ['Unbound','OpenGate']) for(const suffix of ['.2','.1','']) {
        const text=read(home+'/Library/Logs/'+directory+'/events.jsonl'+suffix);
        for(const line of text.split('\n')) {
            let event;try{event=JSON.parse(line);}catch(_){continue;}
            const record={};
            for(const key of allowed)if(typeof event[key]==='string'||typeof event[key]==='number'||typeof event[key]==='boolean')record[key]=event[key];
            // Never carry arbitrary nested widgets, labels, titles, paths or errors.
            if(record.event)records.push(record);
        }
    }
    const installation=[];
    const transactions=argv[2] || '/Library/Application Support/CineWaltz/OpenGate/Transactions';
    const names=ObjC.deepUnwrap($.NSFileManager.defaultManager.contentsOfDirectoryAtPathError(transactions,null)) || [];
    for(const name of names.sort().slice(-30)) {
        // Only installer-generated UUID directories; no arbitrary path traversal.
        if(!/^[A-Fa-f0-9-]{36}$/.test(name))continue;
        const events=[];
        for(const line of read(transactions+'/'+name+'/events.log').split('\n')) {
            const match=line.match(/^(\S+) (preflight|detected|failed|checkpoint|prepared|verifying|installed|rollback|uninstalled)\b/);
            if(match) {
                const record={time:match[1],phase:match[2]};
                for(const field of line.matchAll(/\b(version|build|installed|target|action)=([A-Za-z0-9_.-]+)/g))record[field[1]]=field[2];
                events.push(record);
            }
        }
        if(events.length)installation.push({transaction:name,events:events});
    }
    records.sort((a,b)=>String(a.time||'').localeCompare(String(b.time||'')));
    const result={schema:1,product:'OpenGate',generated:new Date().toISOString(),raw_logs_included:false,installation:installation,
        note:'Event fields are allowlisted; project names, paths, widget contents and arbitrary error text are excluded.',events:records.slice(-10000)};
    const output=$(JSON.stringify(result,null,2)+'\n');
    if(!output.writeToFileAtomicallyEncodingError(destination,true,$.NSUTF8StringEncoding,null))throw Error('Could not save support summary.');
}
