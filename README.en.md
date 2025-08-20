# selectionfwk

#### Description
Starting from API version 20, the newly added Selection Service provides cross-application text processing capabilities as well as the ability to manage system-wide selection applications.

This feature is primarily designed for scenarios such as text translation, content summarization, and term explanation. For example, when a user long-presses a word in foreign-language news, the translation service is automatically triggered, displaying the result in a floating window; when selecting a long paragraph in a reading app, users can leverage the Selection Service to quickly generate a content summary; and in educational apps, specialized terms selected by users can be identified, with instant explanation cards provided accordingly.

#### Repository Path
/foundation/systemabilitymgr/selectionfwk

#### Interface Specification
The interfaces currently provided by the selection input service are all system interfaces and have not yet been opened up to third-party selection input applications. For detailed information on the selection input service interface, please refer to [selectionInput.SelectionExtensionAbility](https://gitcode.com/openharmony/docs/blob/master/zh-cn/application-dev/reference/apis-basic-services-kit/js-apis-selectionInput-selectionExtensionAbility-sys.md) and [selectionInput.selectionManager](https://gitcode.com/openharmony/docs/blob/master/zh-cn/application-dev/reference/apis-basic-services-kit/js-apis-selectionInput-selectionManager-sys.md).

#### Constraints and Restrictions
- The [2-in-1](https://gitcode.com/openharmony/docs/blob/master/en/application-dev/quick-start/module-configuration-file.md#deviceTypes) device supports external keyboards and mice.

- Support for retrieving text selections, with a maximum length of 6,000 bytes.

For more information, please refer to [the development documentation of the selection service application](https://gitcode.com/openharmony/docs/blob/master/zh-cn/application-dev/basic-services/selectionInput/Readme-CN.md).

## Directory Structure

```
/foundation/systemabilitymgr
├── selectionfwk
│   ├── common                                      # Common code 
│   ├── etc                                         # Configuration files for component processes
│   ├── frameworks                                  # Interface implementations
│   │   ├── js/napi                                 # Selection framework NAPI interfaces
│   │   └── native                                  # Native interfaces
│   ├── hiappevent_agent                            # hiappevent platform logging
│   ├── interfaces                                  # IDL interface definition files
│   ├── sa_profile                                  # SA definitions
│   ├── services                                    # Selection framework services
│   ├── test                                        # Test directory
│   │   ├── fuzztest                                # Fuzz testing
│   │   └── unitest                                 # Unit tests for interfaces
│   ├── utils                                       # Core service utility code
```

### Build Steps

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

### Testing Steps

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