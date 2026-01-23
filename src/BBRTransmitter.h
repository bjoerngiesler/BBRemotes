#if !defined(BBRTRANSMITTER_H)
#define BBRTRANSMITTER_H

#include <string>
#include <vector>

#include "BBRTypes.h"
#include "BBRUtils.h"
#include "BBRProtocol.h"

namespace bb {
namespace rmt {

class Protocol;

//! Abstract transmitter superclass
class Transmitter {
public:
    virtual ~Transmitter() {}

     
    /** \defgroup axis_handling Axis handling
     * @{
     * 
     * An **axis** is the concept that BBRemotes uses for the on-the-wire representation of
     * physical controls in a hardware remote control unit.
     * 
     * Transmitters can have fixed or dynamic number of axes. Fixed number is used for static
     * protocols like Monaco, where an a priori fixed number of axes per packet is mapped to an
     * unknown number of droid inputs. Dynamic number of axes is used for protocols like BLE
     * where a fixed number of inputs determines how many axes can be mapped. In these protocols,
     * it is customary to use axisWithName() to implicitly create a virtual axis (although you can 
     * of course create axes separately).
     * 
     * Each axis has a __name__ and a __bit depth__. The name can be changed. Eg if the
     * protocol defines 2 axes and you have a joystick, the protocol may by default name the axes
     * axis0 and axis1, but you may choose to rename them to joyH and joyV. The bit depth cannot
     * be changed, as it's native to the protocol. You can use it to select which controls from your
     * remote you want to map to which axis though. Eg if the protocol defines 2 10-bit and 2 1-bit
     * axes, and you have a joystick and two buttons, you want to set the 10-bit axes to the joystick
     * output and the 1-bit axes to the button output, rather than assigning randomly.
     * 
     * Axes are addressed with AxisID, which is just an uint8_t, and there is a special ID called
     * INVALID_AXIS, which has the value of 255.
     * */
    //! Return the number of axes the protocol supports, or 0 for dynamic axis protocols.
    virtual uint8_t numAxes();
    //! Return the name of the n-th axis.
    virtual const std::string& axisName(AxisID axis);
    //! Set the name of the n-th axis.
    virtual bool setAxisName(AxisID axis, const std::string& name);
    //! Return the axis ID for the given name, or InvalidAxis if not found.
    /**
     * You can use this to implicitly add axes in protocols which can do so, by setting add to true
     * and giving a sane bitdepth.
     */
    virtual AxisID axisWithName(const std::string& name, bool add = false, uint8_t bitDepth = 8);
    //! Return the bitdepth for the given axis.
    virtual uint8_t bitDepthForAxis(AxisID axis);
    //! Whether the protocol can add axes or not.
    virtual bool canAddAxes() { return true; };
    //! Add an axis, if the protocol can do so (otherwise, return INVALID_AXIS).
    virtual AxisID addAxis(const std::string& name, uint8_t bitDepth);
    /**
     * @}
     */

    /** \defgroup value_setting Value setting, transmission and computation
     * @{
     * 
     * This is the group of functions you use to set the values from physical controls to the Transmitter,
     * to send in the next transmit() call. 
     * 
     * `setAxisValue()` and `axisValue()` perform unit conversion, and are the ones that should be used. Supported
     * units are
     * 
     * * `UNIT_UNITY`: `value` in [0..1]
     * * `UNIT_UNITY_CENTERED`: `value` in [-1..1]
     * * `UNIT_DEGREES`: `value` in [0..360]
     * * `UNIT_DEGREES_CENTERED`: `value` in [-180..180]
     * 
     * With this information and the bitdepth assigned to each axis, the input value is converted to an
     * on-the-wire representation. If you want to bypass this (and know what you're doing), you can
     * use `setRawAxisValue()`/ `rawAxisValue()` as well.
     * */

    //! Use this to set the value for the given axis. `unit` provides an idea of the range of `value`.
    virtual bool setAxisValue(AxisID axis, float value, Unit unit);
    //! Use this to set the value for the given axis. `unit` specifies the range of the returned value.
    virtual float axisValue(AxisID axis, Unit unit);
    //! Use this to set the value for the given axis directly. `value` will be capped to the axis's bitdepth.
    virtual bool setRawAxisValue(AxisID axis, uint32_t value);
    //! Use this to return the value for the given axis directly. The returned value will be capped to the axis's bitdepth.
    virtual uint32_t rawAxisValue(AxisID axis);

    //! Compute mix -- for direct transmitters
    /** 
     * `computeMix()` uses the axis-input mix table to compute the value for a given input from
     * the axis values and the mix curves. It is provided in the transmitter for fixed-input protocols like
     * DroidDepot or Sphero. In protocols that do receiver-side mapping, the input value is computed in the 
     * receiver.
     * 
     * AxisMixes always produce a value between -1 and 1, so that's what `computeMix()` returns.
     * */
    virtual float computeMix(const AxisMix& mix);

    //! Transmit data set by `setAxisValue()` family of methods
    /**
     * This is usually called from the Protocol's step() method, but can be called explicitly.
     * */
    virtual bool transmit() = 0;

    /**
     * @}
     */

    
    /** \defgroup primary_handling Handling of Primary / Secondary Transmitters
     * @{
     * 
     * Some protocols have the concept of primary or secondary transmitters (like the Monaco Remotes).
     * These methods are used to handle this mapping.
     */
    virtual bool isPrimary() { return primary_; }
    virtual void setPrimary(bool p) { primary_ = p; }
    /**
     * @}
     */

protected:
    std::vector<Axis> axes_;
    bool primary_;
};

template <typename P> class TransmitterBase: public Transmitter {
public:
    TransmitterBase() = delete; // disallow default constructor

protected:
    friend P;

    TransmitterBase(P* protocol): protocol_(protocol) {}
    P* protocol_;
};

};
};

#endif // BBRTRANSMITTER_H