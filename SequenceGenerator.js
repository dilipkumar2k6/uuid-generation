const os = require('os');

class SequenceGenerator {
    constructor(nodeId){
        this.lastTimestamp = -1;
        this.sequence = 0;

        if(typeof nodeId === 'undefined') {
            // generate nodeid
            this.nodeId = this._createNodeId();
             return this;            
        }
        if(nodeId < 0 | nodeId > SequenceGenerator.maxNodeId) {
            throw new Error('NodeId must be between %d and %d',0, SequenceGenerator.maxNodeId);
        }
        this.nodeId = nodeId;
        return this;
    }
    _createNodeId(){
        let nodeId = '';
        const tuples = [];
        try{
            const networkInterfaces = os.networkInterfaces();
            for(const networkInterface of networkInterfaces) {
                const { mac } = networkInterface; // mac is byte array
                for(const byte of mac) {
                    tuples.push(byte.toString(16));
                }
            }
            nodeId = tuples.join('').hashCode(); // generate node id
        } catch(ex) {
            // generate random number
            nodeId = Math.random() * 10;
        }
        nodeId = nodeId & SequenceGenerator.maxNodeId; 
        return nodeId;
    }
    _timestamp(){
        const epoch = Date.now(); // return epoch milli seconds
        return epoch - SequenceGenerator.CUSTOM_EPOCH;
    }
    _waitNextMillis(currentTimestamp){
        // Block and wait till next millisecond
        while(this.lastTimestamp === currentTimestamp) {
            currentTimestamp = this._timestamp();
        }
        return currentTimestamp;
    }
    nextId(){
        // get current timestamp
        let currentTimestamp = this._timestamp();

        // check to make sure clock is not going back
        if(currentTimestamp < this.lastTimestamp) {
            throw new Error('Invalid system clock');
        }

        if(currentTimestamp === this.lastTimestamp) {
            this.sequence = (this.sequence + 1) & SequenceGenerator.maxSequence;
            if(this.sequence === 0) {
                // sequence exhausted, wait for next milli seconds
                currentTimestamp = this._waitNextMillis(currentTimestamp);
            }
        } else {
            // reset sequence to start with zero for the next milli seconds
            this.sequence = 0;
        }

        // update last timestamp
        this.lastTimestamp = currentTimestamp;

        // perform left shift by 10 + 12 = 22 bits to make space for nodeid and sequence
        let id = currentTimestamp << (SequenceGenerator.NODE_ID_BITS + SequenceGenerator.SEQUENCE_BITS); 

        // perform left shift on noeid by `2 bits to make space for sequence
        const nodeId = this.nodeId << SequenceGenerator.SEQUENCE_BITS;

        // place node id 10 bits on id
        id = id | nodeId;

        // place sequence 12 bits on id
        id = id | this.sequence;
        return id;
    }

}
SequenceGenerator.UNUSED_BITS = 1; // Sign bit, Unused (always set to 0)
SequenceGenerator.EPOCH_BITS = 41;
SequenceGenerator.NODE_ID_BITS = 10;
SequenceGenerator.SEQUENCE_BITS = 12;

SequenceGenerator.maxNodeId = Math.pow(2, SequenceGenerator.NODE_ID_BITS) -1;
SequenceGenerator.maxSequence = Math.pow(2, SequenceGenerator.SEQUENCE_BITS) -1;

// Custom Epoch (January 1, 2015 Midnight UTC = 2015-01-01T00:00:00Z)
SequenceGenerator.CUSTOM_EPOCH = 1420070400000;
