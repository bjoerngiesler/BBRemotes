#if !defined(BBRRECEIVER_H)
#define BBRRECEIVER_H

#include <string>
#include <vector>
#include <functional>

#include "BBRTypes.h"
#include "BBRMixManager.h"

namespace bb {
namespace rmt {

//! Receiver superclass. Not instantiated directly but returned by the protocol's `getReceiver()` method.
class Receiver: public MixManager {
public:
    virtual ~Receiver() {}

    /** \defgroup input_handling Input handling
     * @{
     * 
     * An **input** is the concept that BBRemotes uses for the representation of what a robot or droid can do.
     * Examples for inputs are: Forward speed, turn rate, or some servo angle. An input is computed from one or two
     * on-the wire **axes** by the methods in `MixManager`.
     * 
     * Inputs have unique names, meaning no two inputs can have the same name. The library sets input values (after computing them) 
     * either by directly setting a float variable, or calling a callback with the value as an argument.
     * 
     * Inputs are identified by the `InputID` data type, which is just an `uint8_t`. The value of 255, or `INPUT_INVALID`,
     * is reserved for error handling or cases in which no input ID is given.
     * */
    
    //! Add an input, directly setting a float. Returns input id if OK, INPUT_INVALID if an input for the given name / id exists.
    virtual InputID addInput(const std::string& name, float& variable);
    //! Add an input, calling a callback. Returns input id if OK, INPUT_INVALID if an input for the given name / id exists.
    virtual InputID addInput(const std::string& name, std::function<void(float)> callback);
    //! Returns the name for the given ID.
    virtual const std::string& inputName(InputID input);
    //! Returns the ID for the given name, or INPUT_INVALID .
    virtual InputID inputWithName(const std::string& name);
    //! Returns the number of inputs.
    virtual uint8_t numInputs();

    /**
     * @}
     */

    //! Set a callback to be called when all data has been processed by the framework (all floats set / callbacks called).
    virtual void setDataFinishedCallback(std::function<void(const NodeAddr&, uint8_t)> cb) { dataFinishedCB_ = cb; }

    //! Set a callback to be called with raw data as soon as it has been received (before being processed by the framework).
    virtual void setDataReceivedCallback(std::function<void(const NodeAddr&, uint8_t, const void*, uint8_t)> cb) { dataReceivedCB_ = cb; }

protected:
    struct Input {
        std::string name;
        std::function<void(float)> callback;
    };

    std::vector<Input> inputs_;
    std::function<void(const NodeAddr&, uint8_t, const void*, uint8_t)> dataReceivedCB_ = nullptr;
    std::function<void(const NodeAddr&,uint8_t)> dataFinishedCB_ = nullptr;
};

};
};

#endif // BBRCONFIGURATOR_H