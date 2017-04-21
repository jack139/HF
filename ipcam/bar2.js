print((new Date()).toString())

db = db.getSiblingDB('hf_db')
db.auth('ipcam','ipcam');

var overall = db.stats()

print('db: ' + tojson(overall["db"]));
print('collections: ' + tojson(overall["collections"]));
print('objects: ' + tojson(overall["objects"]));
print('data size: ' + tojson(overall["dataSize"]));
print('storage size: ' + tojson(overall["storageSize"]));
print('indexes: ' + tojson(overall["indexes"]));
print('index size: ' + tojson(overall["indexSize"]));
print('file size: ' + tojson(overall["fileSize"]));

