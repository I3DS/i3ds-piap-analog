///////////////////////////////////////////////////////////////////////////\file
///
///   Copyright 2018 SINTEF AS
///
///   This Source Code Form is subject to the terms of the Mozilla
///   Public License, v. 2.0. If a copy of the MPL was not distributed
///   with this file, You can obtain one at https://mozilla.org/MPL/2.0/
///
////////////////////////////////////////////////////////////////////////////////

#define BOOST_LOG_DYN_LINK
#include <boost/log/trivial.hpp>

#include "xilinx_analog.hpp"

i3ds::XilinxAnalog::Ptr
i3ds::XilinxAnalog::CreateTactile(Context::Ptr context, NodeID id)
{
  Parameters param;

  param.series = 3;
  param.bit_resolution = 12;
  param.type = ADC_TACTILE;

  for (int i = 0; i < param.series; i++)
    {
      param.scale.push_back(20.0 / 4095);
      param.offset.push_back(0.0);
    }

  return std::make_shared<XilinxAnalog>(context, id, param);
}

i3ds::XilinxAnalog::Ptr
i3ds::XilinxAnalog::CreateForceTorque(Context::Ptr context, NodeID id)
{
  Parameters param;

  param.series = 6;
  param.bit_resolution = 12;
  param.type = ADC_TORQUE;

  // Force is the three first
  for (int i = 0; i < param.series / 2; i++)
    {
      param.offset.push_back(-150.0);
      param.scale.push_back(2 * 300.0 / 4095);
    }

  // Torque is the three last
  for (int i = 0; i < param.series / 2; i++)
    {
      param.offset.push_back(-10.0);
      param.scale.push_back(2 * 20.0 / 4095);
    }

  return std::make_shared<XilinxAnalog>(context, id, param);
}

i3ds::XilinxAnalog::Ptr
i3ds::XilinxAnalog::CreateThermistor(Context::Ptr context, NodeID id)
{
  Parameters param;

  param.series = 13;
  param.bit_resolution = 12;
  param.type = ADC_THERMISTOR;

  for (int i = 0; i < param.series; i++)
    {
      // TODO: Set correct scale and offset for thermistors.
      param.scale.push_back(2 * 20.0 / 4095);
      param.offset.push_back(0.0);
    }

  return std::make_shared<XilinxAnalog>(context, id, param);
}


i3ds::XilinxAnalog::XilinxAnalog(Context::Ptr context, NodeID node, const Parameters& param)
  : Analog(node, param.series),
    param_(param),
    sampler_(std::bind(&i3ds::XilinxAnalog::send_sample, this, std::placeholders::_1)),
    publisher_(context, node),
    batches_(0)

{
  if (adc_initialize() != 0) {
    BOOST_LOG_TRIVIAL(error) << "Could not initialize ADC!";
  }

  BOOST_LOG_TRIVIAL(info) << "Create Xilinx analog sensor with NodeID: " << node;

  MeasurementTopic::Codec::Initialize(frame_);

  set_device_name("PIAP analog sensor");
}

i3ds::XilinxAnalog::~XilinxAnalog()
{
  BOOST_LOG_TRIVIAL(info) << "Destroy Xilinx analog sensor with NodeID: " << node();
}

void
i3ds::XilinxAnalog::do_activate()
{
  BOOST_LOG_TRIVIAL(info) << "Xilinx analog sensor with NodeID: " << node() << " do_activate()";
}

void
i3ds::XilinxAnalog::do_start()
{
  BOOST_LOG_TRIVIAL(info) << "Xilinx analog sensor with NodeID: " << node() << " do_start()";
  batches_ = 0;
  sampler_.Start(period() / batch_size());
}

void
i3ds::XilinxAnalog::do_stop()
{
  BOOST_LOG_TRIVIAL(info) << "Xilinx analog sensor with NodeID: " << node() << " do_stop()";
  sampler_.Stop();
}

void
i3ds::XilinxAnalog::do_deactivate()
{
  BOOST_LOG_TRIVIAL(info) << "Xilinx analog sensor with NodeID: " << node() << " do_deactivate()";
}

bool
i3ds::XilinxAnalog::is_sampling_supported(SampleCommand sample)
{
  BOOST_LOG_TRIVIAL(info) << "Xilinx analog sensor with NodeID: " << node() << " is_period_supported()";
  return (1 <= sample.batch_size && sample.batch_size <= 100)
         && (10000 <= sample.period && sample.period <= 10000000);
}

bool
i3ds::XilinxAnalog::send_sample(unsigned long timestamp_us)
{
  BOOST_LOG_TRIVIAL(trace) << "Xilinx analog sensor with NodeID: " << node() << " samples " << timestamp_us;

  std::vector<float> value = read_adc();

  const int offset = batches_ * param_.series;

  for (int i = 0; i < param_.series; i++)
    {
      frame_.samples.arr[offset + i] = value[i];
    }

  batches_++;

  if (batches_ >= batch_size())
    {
      BOOST_LOG_TRIVIAL(trace) << "Xilinx analog sensor with NodeID: " << node() << " sends sample at " << timestamp_us;
      BOOST_LOG_TRIVIAL(trace) << batches_ << " batches and " << param_.series << " series";

      frame_.attributes.timestamp = timestamp_us;
      frame_.attributes.validity = sample_valid;

      frame_.samples.nCount = batches_ * param_.series;
      frame_.series = param_.series;
      frame_.batch_size = batches_;

      publisher_.Send<MeasurementTopic>(frame_);

      batches_ = 0;
    }

  return true;
}

std::vector<float>
i3ds::XilinxAnalog::read_adc()
{
  std::vector<float> value(param_.series);

  BOOST_LOG_TRIVIAL(trace) << "Reading values raw/converted:";
  for (int i = 0; i < param_.series; i++)
    {
      const uint32_t adc = adc_read_value(param_.type, i);

      // Apply scale and offset to discrete value.
      value[i] = param_.scale[i] * adc + param_.offset[i];
      BOOST_LOG_TRIVIAL(trace) << " " << adc << " -> " << value[i];
    }


  return value;
}
