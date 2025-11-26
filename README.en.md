# selectionfwk

## Description
Starting from API version 20, a new word selection service module has been introduced, which enables cross-application text processing and system-level management of word selection applications.

This module is primarily designed for scenarios such as text translation, content summarization, and terminology explanation. For instance, when a user selects a word from foreign news articles, the translated result can be displayed. In reading applications, selected content can be quickly summarized. In learning applications, it can identify selected professional terms and provide instant explanation cards.

### Framework Principles
The word selection service primarily relies on domains such as multimodal and clipboard to provide global word selection functionality externally. The core business process of this service is mainly divided into the following steps:
- Step 1: The word selection service can be started or stopped on demand based on the switch in the system settings. When activated, it will launch the user-selected word selection app (if no selection is made, the earliest installed word selection app will be launched by default). Corresponding to the numbered items ① and ② in the figure below.
- Step 2: During runtime, it monitors multimodal events to detect user word selection actions. Once the user's word selection operation is recognized, it transmits word selection markers to the clipboard and registers a callback function. Simultaneously, it injects simulated CTRL+C operations into the target application via multimodal interfaces. This corresponds to sequence numbers ③④⑤⑥ in the figure below.
- Step 3: After receiving the CTRL+C command, the highlighted word application triggers the copy operation, writing the currently selected content to the clipboard. Upon receiving the data, the clipboard transmits the text content back to the highlighting service. Finally, the highlighting service passes the content to the highlighted word application, which processes the corresponding business logic and displays the highlighting window. This corresponds to the numbered steps ⑦, ⑧, and ⑨ in the diagram below.

![Architectural Diagram of the Selection Service Framework](figures/selection-service-schematic-en.PNG)

### Repository Path
/foundation/systemabilitymgr/selectionfwk

### Interface Specification
The interfaces currently provided by the selection input service are all system interfaces and have not yet been opened up to third-party selection input applications. For detailed information on the selection input service interface, please refer to [selectionInput.SelectionExtensionAbility](https://gitcode.com/openharmony/docs/blob/master/zh-cn/application-dev/reference/apis-basic-services-kit/js-apis-selectionInput-selectionExtensionAbility-sys.md) and [selectionInput.selectionManager](https://gitcode.com/openharmony/docs/blob/master/zh-cn/application-dev/reference/apis-basic-services-kit/js-apis-selectionInput-selectionManager-sys.md).

### Constraints and Restrictions
- The [PC/2-in-1](https://gitcode.com/openharmony/docs/blob/master/en/application-dev/quick-start/module-configuration-file.md#deviceTypes) device supports external keyboards and mice.

- Support for retrieving text selections, with a maximum length of 6,000 bytes.

- Supports use on extended screens, does not support cross device use.

- The current interfaces are system interfaces and are planned to be opened as public interfaces in API version 22.

For more information, please refer to [the development documentation of the selection service application](https://gitcode.com/openharmony/docs/blob/master/zh-cn/application-dev/basic-services/selectionInput/Readme-CN.md).

## Directory Structure

```
/foundation/systemabilitymgr
├── selectionfwk
│   ├── common                                      # Common code 
│   ├── etc                                         # Configuration files for component processes
│   ├── figures                                     # Framework diagram
│   ├── frameworks                                  # Interface implementations
│   │   ├── js/napi                                 # Selection framework NAPI interfaces
│   │   └── native                                  # Native interfaces
│   ├── hiappevent_agent                            # hiappevent platform logging
│   ├── interfaces                                  # IDL interface definition files
│   ├── sa_profile                                  # SA definitions
│   ├── services                                    # Selection framework services
│   ├── sysevent                                    # hisysevent platform logging
│   ├── test                                        # Test directory
│   │   ├── fuzztest                                # Fuzz testing
│   │   └── unitest                                 # Unit tests for interfaces
│   ├── utils                                       # Core service utility code
```

## Build Steps

- Full Build

    After modifying the build.gn file, execute:
    ```
    $ ./build.sh --product-name rk3568 --ccache
    ```
    If the build.gn file is unmodified, execute:
    ```
    $ ./build.sh --product-name rk3568 --ccache --fast-rebuild
    ```

- Partial Build

    ```
    $ ./build.sh --product-name rk3568 --ccache --build-target selectionfwk
    ```

## Testing Steps

1. Testing Methods

    Check the service process:
    ```
    # ps -ef | grep selection
    ```
    Start the selection service:
    ```
    # param set sys.selection.switch on
    ```
    Stop the selection service:
    ```
    # param set sys.selection.switch off
    ```
    Switch the selection application:
    ```
    # param set sys.selection.app com.selection.selectionapplication/SelectionExtensionAbility
    ```
    Set the selection trigger method:
    ```
    # param set sys.selection.trigger "immediate"
    ```

2. Log Retrieval Commands

    Enable debug logs:
    ```
    # hilog -b D
    ```
    Filter logs:
    ```
    # hilog -T SELECTION_SERVICE
    ```

## Contribution

1.  Fork the repository
2.  Create Feat_xxx branch
3.  Commit your code
4.  Create Pull Request