// Copyright (c) 2023 Mark Wharton
// https://opensource.org/license/mit/

export class JSONParser @ "xs_jsonparser_destructor" {
    // Constructor for the JSONParser class, invokes "xs_jsonparser_constructor".
    constructor(options) @ "xs_jsonparser_constructor";

    // Getter for the 'data' property, representing the matcher.
    get data() { return this.vpt.data; }

    // Getter for the 'root' property, representing the tree.
    get root() { return this.vpt.root; }

    // Getter for the 'status' property, invokes "xs_jsonparser_status".
    get status() @ "xs_jsonparser_status";

    // Closes the JSONParser instance, invokes "xs_jsonparser_close".
    close() @ "xs_jsonparser_close";

    // Creates a new JSONTree instance.
    makeJSONTree() {
        return new JSONTree({});
    }

    // Creates a new VPT instance with an optional matcher.
    makeVPT(matcher = undefined) {
        return new VPT({ matcher });
    }

    // Receives input string and parses it from the specified start to end position, invokes "xs_jsonparser_receive".
    receive(string, start, end) @ "xs_jsonparser_receive";
}
// Static properties representing parsing outcomes.
JSONParser.failure = -1;
JSONParser.receive = 0;
JSONParser.success = 1;

export class Matcher {
    // Constructor for the Matcher class, validates options.
    constructor(options) {
        if (options === undefined)
            throw new Error("invalid options");
        this.begin = options.begin; // Beginning function
        this.match = options.match; // Matching function
    }
}

export class Node {
    // Constructor for the Node class, validates options and establishes previous link.
    constructor(options) {
        if (options === undefined)
            throw new Error("invalid options");
        this.type = options.type;
        if (options.prev !== undefined) {
            this.prev = options.prev;
            this.prev.next = this;
        }
    }

    // Getter for the 'data' property, initializes an empty object if not present.
    get data() {
        if (this.$ === undefined)
            this.$ = {};
        return this.$;
    }

    // Moves 'count' steps up in the linked list, filters by nodeType and/or nodeText.
    up(count = 1, nodeType = undefined, nodeText = undefined) {
        let node = this;
        while (node && count--) {
            node = node.prev;
        }
        return (node && (!nodeType || (node.type === nodeType && (!nodeText || node.text === nodeText)))) ? node : undefined;
    }
}

// Enumeration representing node types.
export const NodeType = Object.freeze({
    null: 1,
    false: 2,
    true: 3,
    number: 4,
    string: 5,
    array: 6,
    object: 7,
    field: 8,
    root: 9
});

export class VPT {
    // Constructor for the VPT class, initializes with an optional matcher and invokes 'begin'.
    constructor(options) {
        if (options === undefined)
            throw new Error("invalid options");
        this.node = this.makeNode(NodeType.root);
        this.matcher = options.matcher;
        this.matcher?.begin?.(this);
    }

    // Creates a new Node with the specified type and previous node.
    makeNode(nodeType, prev = undefined) {
        return new Node({ type: nodeType, prev });
    }

    // Pops the current node if its type matches 'nodeType', invokes 'match', and updates the current node.
    pop(nodeType) {
        if (this.node.type !== nodeType)
            return false;
        this.matcher?.match?.(this, this.node);
        this.node = this.node.prev;
        // Keeps down node (node.next) for any field with a primitive value, else drops it (provides no value).
        if (!(this.node.type === NodeType.field && this.node.next.type <= NodeType.string))
            this.node.next = undefined; // Drops down node (provides no value).
        return true;
    }

    // Pushes a new node with the specified type onto the stack.
    push(nodeType) {
        this.node = this.makeNode(nodeType, this.node);
    }

    // Sets the text property of the current node.
    setText(text) {
        this.node.text = text;
    }
}

class JSONTree extends VPT {
    // Constructor for the JSONTree class, initializes with an empty root node and a stack.
    constructor(options) {
        if (options === undefined)
            throw new Error("invalid options");
        super(options);
        this.root = this.node;
        this.stack = [];
    }

    // Creates a new TreeNode with the specified type.
    makeNode(nodeType) {
        return new TreeNode({ type: nodeType });
    }

    // Pops the current node if its type matches 'nodeType', handles field node rejection, and updates the current node.
    pop(nodeType) {
        if (this.node.type !== nodeType)
            return false;
        let node = this.stack.pop();
        // Fields are pushed before the name is known, so we have to check and balance the tree.
        // Prunes field node that was rejected because it failed to match any of the keys.
        if (this.node.type === NodeType.field && this.node.text === undefined && this.node.length === 0) {
            let index = node.indexOf(this.node);
            node.splice(index, 1);
        }
        this.node = node;
        return true;
    }

    // Pushes a new node with the specified type onto the stack and updates the current node.
    push(nodeType) {
        this.stack.push(this.node);
        let node = this.makeNode(nodeType);
        this.node.push(node);
        this.node = node;
    }
}

class TreeNode extends Array {
    // Constructor for the TreeNode class, validates options and establishes type property.
    constructor(options) {
        if (options === undefined)
            throw new Error("invalid options");
        super();
        this.type = options.type;
    }

    // Getter for the 'value' property, returns the corresponding JavaScript value based on node type.
    get value() {
        switch (this.type) {
            case NodeType.null:
                return null;
            case NodeType.false:
                return false;
            case NodeType.true:
                return true;
            case NodeType.number:
                // 'this.text' may be undefined if root.value is requested before parsing is complete.
                return this.text?.includes(".") ? parseFloat(this.text) : parseInt(this.text, 10);
            case NodeType.string:
                // 'this.text' may be undefined if root.value is requested before parsing is complete.
                return this.text?.valueOf();
            case NodeType.array: {
                let value = [];
                this.forEach(node => {
                    value.push(node.value);
                });
                return value;
            }
            case NodeType.object: {
                let value = {};
                this.forEach(node => {
                    // 'node.text' may be undefined, and the node array may be empty
                    // if root.value is requested before parsing is complete.
                    if (node.text !== undefined && node.length > 0)
                        value[node.text] = node[0].value;
                });
                return value;
            }
            case NodeType.field:
                return;
            case NodeType.root:
                return this[0]?.value;
        }
    }
}
