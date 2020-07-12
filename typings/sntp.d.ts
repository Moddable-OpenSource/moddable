declare module "sntp" {
  type SNTPOptions = {
    host: string
  };
  type SNTPUnableToRetrieveTime = -1;
  type SNTPTimeRetrieved = 1;
  type SNTPRetry = 2;
  type SNTPCallbackMessage = (
    SNTPUnableToRetrieveTime |
    SNTPTimeRetrieved |
    SNTPRetry
  )
  type SNTPCallback = (message: SNTPCallbackMessage, value?: number) => void;
  class SNTP {
    constructor(options: SNTPOptions, callback: SNTPCallback);
  }
  export {SNTP as default};
}
