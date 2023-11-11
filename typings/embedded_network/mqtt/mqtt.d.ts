// Adapted from https://github.com/mqttjs/MQTT.js
//
// The MIT License (MIT)
//
// Copyright (c) 2015-2016 MQTT.js contributors
// *MQTT.js contributors listed at <https://github.com/mqttjs/MQTT.js#contributors>*
// Copyright 2011-2014 by Adam
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software
// and associated documentation files (the "Software"), to deal in the Software without restriction,
// including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,
// subject to the following conditions:
// The above copyright notice and this permission notice shall be included in all copies or
// substantial portions of the Software.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
// BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

declare module "embedded:network/mqtt/mqtt" {

type QoS = 0 | 1 | 2

export interface IClientOptions /*extends ISecureClientOptions*/ {
  // port?: number // port is made into a number subsequently
  // host?: string // host does NOT include port
  // hostname?: string
  // path?: string
  // protocol?: "wss" | "ws" | "mqtt" | "mqtts" | "tcp" | "ssl" | "wx" | "wxs"

  // wsOptions?: ClientOptions | ClientRequestArgs
  keepalive?: number // 10 seconds, set to 0 to disable
  clientId?: string // 'mqttxs_' + Math.random().toString(16).substr(2, 8)
  // protocolId?: string // 'MQTT'
  // protocolVersion?: number //4
  clean?: boolean //true, set to false to receive QoS 1 and 2 messages while offline
  reconnectPeriod?: number //1000 milliseconds, interval between two reconnections
  connectTimeout?: number //30 * 1000 milliseconds, time to wait before a CONNACK is received
  username?: string //the username required by your broker, if any
  password?: string //the password required by your broker, if any
  // queueQoSZero?: boolean
  // reschedulePings?: boolean
  // servers?: Array<{
  //   host: string
  //   port: number
  //   protocol?: "wss" | "ws" | "mqtt" | "mqtts" | "tcp" | "ssl" | "wx" | "wxs"
  // }>
  resubscribe?: boolean //  true, set to false to disable re-subscribe functionality
  //  a message that will sent by the broker automatically when the client disconnect badly
  will?: {
    topic: string //  the topic to publish
    payload: ArrayBuffer | string //  the message to publish
    qos: QoS //  the QoS
    retain: boolean //  the retain flag
    // properies object of will
    // properties?: {
    //   willDelayInterval?: number
    //   payloadFormatIndicator?: boolean
    //   messageExpiryInterval?: number
    //   contentType?: string
    //   responseTopic?: string
    //   correlationData?: ArrayBuffer
    //   userProperties?: UserProperties
    // }
  }
  // transformWsUrl?: (url: string, options: IClientOptions, client: MqttClient) => string
  // properties?: {
  //   sessionExpiryInterval?: number
  //   receiveMaximum?: number
  //   maximumPacketSize?: number
  //   topicAliasMaximum?: number
  //   requestResponseInformation?: boolean
  //   requestProblemInformation?: boolean
  //   userProperties?: UserProperties
  //   authenticationMethod?: string
  //   authenticationData?: Buffer
  // }
  // messageIdProvider?: IMessageIdProvider
}

export interface IClientPublishOptions {
  qos?: QoS
  retain?: boolean
  dup?: boolean // whether or not mark a message as duplicate
  // MQTT 5.0 properties object
  // properties?: {
  //   payloadFormatIndicator?: boolean
  //   messageExpiryInterval?: number
  //   topicAlias?: number
  //   responseTopic?: string
  //   correlationData?: Buffer
  //   userProperties?: UserProperties
  //   subscriptionIdentifier?: number
  //   contentType?: string
}

export interface IClientSubscribeOptions {
  // the QoS
  qos: QoS
  // nl?: boolean // no local flag
  // rap?: boolean // Retain As Published flag
  // rh?: number // Retain Handling option
  //  MQTT 5.0 properies object of subscribe
  // properties?: {
  //   subscriptionIdentifier?: number
  //   userProperties?: UserProperties
  // }
}

export interface ISubscriptionGrant {
  topic: string // is a subscribed to topic
  qos: QoS // is the granted qos level on it
  //     nl?: boolean // no local flag
  //     rap?: boolean // Retain As Published flag
  //     rh?: number // Retain Handling option
}
// export interface ISubscriptionRequest {
//   topic: string // is a subscribed to topic
//   qos: QoS // is the granted qos level on it
//   nl?: boolean // no local flag
//   rap?: boolean // Retain As Published flag
//   rh?: number // Retain Handling option
// }

export interface ISubscriptionMap {
  // object which has topic names as object keys and as value the options, like {'test1': {qos: 0}, 'test2': {qos: 2}}.
  [topic: string]: {
    qos: QoS
    // nl?: boolean
    // rap?: boolean
    // rh?: number
  }
}

//export type OnConnectCallback = (packet: IConnackPacket) => void
export type OnConnectCallback = () => void
//export type OnDisconnectCallback = (packet: IDisconnectPacket) => void
export type ClientSubscribeCallback = (err: Error, granted: ISubscriptionGrant[]) => void
export type OnMessageCallback = (
  topic: string,
  payload: ArrayBuffer
  //packet: IPublishPacket
) => void
//export type OnPacketCallback = (packet: Packet) => void
export type OnCloseCallback = () => void
export type OnErrorCallback = (error: Error) => void
//export type PacketCallback = (error?: Error, packet?: Packet) => any
export type PacketCallback = (error?: Error) => any
//export type CloseCallback = (error?: Error) => void
export type CloseCallback = () => void

/**
 * MqttClient constructor
 *
 * @param {Stream} stream - stream
 * @param {Object} [options] - connection options
 * (see Connection#connect)
 */
export class Client /*extends events.EventEmitter*/ {
  public connected: boolean
  // public disconnecting: boolean
  // public disconnected: boolean
  public reconnecting: boolean
  // public options: IClientOptions
  // public queueQoSZero: boolean

  constructor(url: string, options: IClientOptions)

  public on(event: "connect", cb: OnConnectCallback): this
  public on(event: "message", cb: OnMessageCallback): this
  //public on(event: "packetsend" | "packetreceive", cb: OnPacketCallback): this
  //public on(event: "disconnect", cb: OnDisconnectCallback): this
  public on(event: "error", cb: OnErrorCallback): this
  public on(event: "close", cb: OnCloseCallback): this
  public on(event: "end" | "reconnect" | "offline", cb: () => void): this
  //public on(event: "end" | "reconnect" | "offline" | "outgoingEmpty", cb: () => void): this
  public on(event: string, cb: Function): this

  // public once(event: "connect", cb: OnConnectCallback): this
  // public once(event: "message", cb: OnMessageCallback): this
  // public once(event: "packetsend" | "packetreceive", cb: OnPacketCallback): this
  // public once(event: "disconnect", cb: OnDisconnectCallback): this
  // public once(event: "error", cb: OnErrorCallback): this
  // public once(event: "close", cb: OnCloseCallback): this
  // public once(event: "end" | "reconnect" | "offline" | "outgoingEmpty", cb: () => void): this
  // public once(event: string, cb: Function): this

  /**
   * publish - publish <message> to <topic>
   *
   * @param {String} topic - topic to publish to
   * @param {(String|Buffer)} message - message to publish
   *
   * @param {Object}    [opts] - publish options, includes:
   *   @param {Number}  [opts.qos] - qos level to publish on
   *   @param {Boolean} [opts.retain] - whether or not to retain the message
   *   @param {Function}[opts.cbStorePut] - function(){}
   *       called when message is put into `outgoingStore`
   *
   * @param {Function} [callback] - function(err){}
   *    called when publish succeeds or fails
   *
   * @returns {Client} this - for chaining
   * @api public
   *
   * @example client.publish('topic', 'message')
   * @example
   *     client.publish('topic', 'message', {qos: 1, retain: true})
   * @example client.publish('topic', 'message', console.log)
   */
  public publish(
    topic: string,
    message: string | ArrayBuffer,
    opts: IClientPublishOptions,
    callback?: PacketCallback
  ): this
  public publish(topic: string, message: string | ArrayBuffer, callback?: PacketCallback): this

  /**
   * subscribe - subscribe to <topic>
   *
   * @param {String, Array, Object} topic - topic(s) to subscribe to, supports objects in the form {'topic': qos}
   * @param {Object} [opts] - optional subscription options, includes:
   * @param  {Number} [opts.qos] - subscribe qos level
   * @param {Function} [callback] - function(err, granted){} where:
   *    {Error} err - subscription error (none at the moment!)
   *    {Array} granted - array of {topic: 't', qos: 0}
   * @returns {MqttClient} this - for chaining
   * @api public
   * @example client.subscribe('topic')
   * @example client.subscribe('topic', {qos: 1})
   * @example client.subscribe({'topic': 0, 'topic2': 1}, console.log)
   * @example client.subscribe('topic', console.log)
   */
  public subscribe(
    topic: string | string[],
    opts: IClientSubscribeOptions,
    callback?: ClientSubscribeCallback
  ): this
  public subscribe(
    topic: string | string[] | ISubscriptionMap,
    callback?: ClientSubscribeCallback
  ): this

  /**
   * unsubscribe - unsubscribe from topic(s)
   *
   * @param {String, Array} topic - topics to unsubscribe from
   * @param {Object} opts - opts of unsubscribe
   * @param {Function} [callback] - callback fired on unsuback
   * @returns {MqttClient} this - for chaining
   * @api public
   * @example client.unsubscribe('topic')
   * @example client.unsubscribe('topic', console.log)
   * @example client.unsubscribe('topic', opts, console.log)
   */
  public unsubscribe(topic: string | string[], opts?: Object, callback?: PacketCallback): this

  /**
   * end - close connection
   *
   * @returns {MqttClient} this - for chaining
   * @param {Boolean} force - do not wait for all in-flight messages to be acked
   * @param {Object} opts - opts disconnect
   * @param {Function} cb - called when the client has been closed
   *
   * @api public
   */
  public end(force?: boolean, opts?: Object, cb?: CloseCallback): this

  /**
   * removeOutgoingMessage - remove a message in outgoing store
   * the outgoing callback will be called withe Error('Message removed') if the message is removed
   *
   * @param {Number} mid - messageId to remove message
   * @returns {MqttClient} this - for chaining
   * @api public
   *
   * @example client.removeOutgoingMessage(client.getLastMessageId());
   */
  //public removeOutgoingMessage(mid: number): this

  /**
   * reconnect - connect again using the same options as connect()
   *
   * @param {Object} [opts] - optional reconnect options, includes:
   *    {Store} incomingStore - a store for the incoming packets
   *    {Store} outgoingStore - a store for the outgoing packets
   *    if opts is not given, current stores are used
   *
   * @returns {MqttClient} this - for chaining
   *
   * @api public
   */
  public reconnect(): this
  //public reconnect(opts?: IClientReconnectOptions): this

  /**
   * Handle messages with backpressure support, one at a time.
   * Override at will.
   *
   * @param packet packet the packet
   * @param callback callback call when finished
   * @api public
   */
  //public handleMessage(packet: Packet, callback: PacketCallback): void

  /**
   * Handle auth packages for MQTT 5 enhanced authentication methods such
   * as challenge response authentication.
   *
   * Challenge-response authentication flow would look something like this:
   *
   * --> CONNECT | authMethod = "mathChallenge" -->
   * <-- AUTH | authMethod = "mathChallenge", authData = "12 + 34" <--
   * --> AUTH | authMethod = "mathChallenge", authData = "46" -->
   * <-- CONNACK | reasonCode = SUCCESS <--
   *
   * This form of authentication has several advantages over traditional
   * credential-based approaches. For instance authentication without the direct
   * exchange of authentication secrets.
   *
   * @param packet the auth packet to handle
   * @param callback call when finished
   * @api public
   */
  //public handleAuth(packet: IAuthPacket, callback: PacketCallback): void

  /**
   * getLastMessageId
   */
  public getLastMessageId(): number
}

}
