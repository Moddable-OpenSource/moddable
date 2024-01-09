// Copyright (c) 2023-2024 Mark Wharton
// https://opensource.org/license/mit/

export class JSONParser @ "xs_jsonparser_destructor" {
    // Constructor for the JSONParser class, invokes "xs_jsonparser_constructor".
    constructor(options) @ "xs_jsonparser_constructor"

    // Getter for the 'data' property, representing the matcher or patterns.
    get data() { return this.vpt.data; }

    // Getter for the 'root' property, representing the tree.
    get root() { return this.vpt.root; }

    // Getter for the 'status' property, invokes "xs_jsonparser_status".
    get status() @ "xs_jsonparser_status"

    // Closes the JSONParser instance, invokes "xs_jsonparser_close".
    close() @ "xs_jsonparser_close"

    // Creates a new JSONTree instance.
    makeJSONTree(options) {
        return new JSONTree(options);
    }

    // Creates a new VPT instance.
    makeVPT(options) {
        return new VPT(options);
    }

    // Receives input string and parses it from the specified start to end position, invokes "xs_jsonparser_receive".
    receive(string, start, end) @ "xs_jsonparser_receive"
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
        this.begin = options.begin; // begin function
        this.match = options.match; // match function
        this.setup = undefined; // nop setup function
    }

    // Executes the begin function if defined.
    onBegin(vpt, node) {
        this.begin?.(vpt);
    }

    // Executes the match function if defined.
    onMatch(vpt, node) {
        this.match?.(vpt, node);
    }

    // Executes the setup function if defined.
    onSetup(vpt, node) {
        this.setup?.(vpt, node);
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

export class Pattern extends Matcher {
    // Constructor for the Pattern class, validates options and establishes pattern value items.
    constructor(options) {
        super(options);
        this.setup = options.setup; // setup function
        this.value = options.value; // pattern value string
        const itemDelimiter = options.itemDelimiter || "/";
        const nameDelimiter = options.nameDelimiter || ":";
        this.items = this.value.split(itemDelimiter).map((item, index) => {
            if (index === 0 && item === "" && this.value.startsWith(itemDelimiter))
                return { type: NodeType.root };
            const pair = item.split(nameDelimiter);
            const text = pair[1]; // field name
            switch (pair[0]) {
                case "null":
                    return { type: NodeType.null };
                case "false":
                    return { type: NodeType.false };
                case "true":
                    return { type: NodeType.true };
                case "number":
                    return { type: NodeType.number, text };
                case "string":
                    return { type: NodeType.string, text };
                case "array":
                    return { type: NodeType.array };
                case "object":
                    return { type: NodeType.object };
                case "field":
                    return { type: NodeType.field, text };
                case "root":
                    return { type: NodeType.root };
            }
        }).reverse().filter(item => item !== undefined);
    }

    // Gets an array of field names from the pattern.
    get names() {
        const value = [];
        this.items.forEach(item => {
            if (item !== undefined && item.type === NodeType.field)
                value.push(item.text);
        });
        return value;
    }
}

export class VPT {
    // The VPT class constructor initializes with an optional matcher or patterns, always invoking 'begin' for the matcher and only for patterns matching the root pattern.
    // Note: Instances of VPT with a matcher or patterns cannot be shared; VPT constructor must execute for each new parser.
    constructor(options) {
        if (options === undefined)
            throw new Error("invalid options");
        this.node = this.makeNode(NodeType.root);
        this.matcher = options.matcher;
        this.patterns = options.patterns;
        this.doEvent("onBegin");
    }

    // Execute 'matcher events' unconditionally and 'pattern events' selectively when nodes match a pattern.
    doEvent(event) {
        this.matcher?.[event](this, this.node);
        this.patterns?.forEach(pattern => {
            let node = this.node;
            for (const item of pattern.items) {
                if (item !== undefined && node !== undefined) {
                    if (item.type === node.type && (item.text === undefined || item.text === node.text)) {
                        if (node.type === NodeType.root)
                            break;
                        node = node.prev;
                    }
                    else {
                        node = undefined;
                        break;
                    }
                }
            }
            if (node !== undefined)
                pattern[event](this, this.node);
        });
    }

    // Creates a new Node with the specified type and previous node.
    makeNode(nodeType, prev = undefined) {
        return new Node({ type: nodeType, prev });
    }

    /*
    // Uncomment this helper method for trace messages when needed.
    nodeText(nodeType) {
        switch (nodeType) {
            case NodeType.null:
                return "null";
            case NodeType.false:
                return "false";
            case NodeType.true:
                return "true";
            case NodeType.number:
                return "number";
            case NodeType.string:
                return "string";
            case NodeType.array:
                return "array";
            case NodeType.object:
                return "object";
            case NodeType.field:
                return "field";
            case NodeType.root:
                return "root";
        }
    }
    */

    // Pops the current node if its type matches 'nodeType', invokes 'match', and updates the current node.
    pop(nodeType) {
        if (this.node.type !== nodeType)
            return false;
        this.doEvent("onMatch");
        this.node = this.node.prev;
        // Keeps down node (node.next) for any field with a primitive value, else drops it (provides no value).
        if (!(this.node.type === NodeType.field && this.node.next.type <= NodeType.string))
            this.node.next = undefined; // Drops down node (provides no value).
        return true;
    }

    // Pushes a new node with the specified type onto the stack and invokes 'setup'.
    push(nodeType) {
        this.node = this.makeNode(nodeType, this.node);
        this.doEvent("onSetup");
    }

    // Sets the text property of the current node and invokes 'setup'.
    setText(text) {
        this.node.text = text;
        this.doEvent("onSetup");
    }
}

class JSONTree extends VPT {
    // Constructor for the JSONTree class, initializes with an empty root node and a stack.
    constructor(options) {
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
