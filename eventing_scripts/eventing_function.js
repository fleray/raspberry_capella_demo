function OnUpdate(doc, meta) {
    log("New data insertion", meta.id)
    var id = doc.sensor + ":" + Math.trunc(doc.timestamp / 60000)
    if(tgt[id]){
        var agg = tgt[id]
        
        if(doc.timestamp > agg.ts_end){
            agg.ts_end = doc.timestamp
        } else if(doc.timestamp < agg.ts_start){
            agg.ts_start = doc.timestamp
        }
        
        agg.ts_data.push([doc.timestamp, doc.temperature])
        
        tgt[id] = agg

        
    } else {
        tgt[id] = {
            "ts_start" : doc.timestamp,
            "ts_end" : doc.timestamp,
            // "ts_interval" : 10,
            "device" : doc.sensor,
            "ts_data" : [[doc.timestamp, doc.temperature]]
        }
    }
}
