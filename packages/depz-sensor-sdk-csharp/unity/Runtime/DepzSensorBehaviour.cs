#if UNITY_5_3_OR_NEWER
#nullable enable
using System;
using Depz.Sensor.Devices;
using UnityEngine;

namespace Depz.Sensor.Unity
{
    /// <summary>
    /// Sample component: opens the first DEPZ board (or the one <see cref="Port"/>
    /// / <see cref="Serial"/> names) in Start(), drains its event queue from
    /// Update() and closes it in OnDestroy(). Derive from it and override
    /// <see cref="OnOpened"/> and <see cref="OnEvent"/>, or copy it. The device's
    /// reader thread must never touch Unity objects, so everything the SDK
    /// delivers is polled here, on the main thread, from <see cref="Subscription{T}"/>
    /// queues (<c>TryTake</c> never blocks).
    /// </summary>
    public class DepzSensorBehaviour : MonoBehaviour
    {
        [Tooltip("COM port to open (COM5). Empty: discover by USB id.")]
        public string Port = "";

        [Tooltip("USB serial of the board to open. Empty: the smallest serial.")]
        public string Serial = "";

        private Subscription<DeviceEvent>? _events;

        /// <summary>The open device, or null before Start() and after a failed one.</summary>
        public DepzDevice? Device { get; private set; }

        /// <summary>Open the device and hand it to <see cref="OnOpened"/>. Whatever fails after the open — a
        /// request, <see cref="OnOpened"/> itself — closes the device again and logs: the component is either
        /// started or has no device.</summary>
        protected virtual void Start()
        {
            DepzDevice device;
            try
            {
                device = OpenDevice();
            }
            catch (DepzException e)
            {
                Debug.LogError($"DEPZ: {e.Message}", this);
                return;
            }
            try
            {
                _events = device.Events();
                Debug.Log($"DEPZ: {device.GetType().Name} on {device.Port}: {device.GetSoftwareName()}", this);
                Device = device;
                OnOpened(device);
            }
            catch (Exception e)
            {
                _events?.Dispose();
                _events = null;
                Device = null;
                device.Close();
                if (e is DepzException)
                    Debug.LogError($"DEPZ: {e.Message}", this);
                else
                    Debug.LogException(e, this);
            }
        }

        /// <summary>Open the device Start() runs on: <see cref="Port"/>, else the board of <see cref="Serial"/>, else
        /// the first one. Override to open it another way (a replay link, say).</summary>
        protected virtual DepzDevice OpenDevice() =>
            Port.Length > 0
                ? DepzDevicesUnity.OpenPort(Port)
                : DepzDevicesUnity.OpenDevice(Serial.Length > 0 ? Serial : null);

        protected virtual void Update()
        {
            if (_events is null)
                return;
            while (_events.TryTake(out DeviceEvent ev))
                OnEvent(ev);
        }

        protected virtual void OnDestroy()
        {
            _events?.Dispose();
            _events = null;
            Device?.Close();
            Device = null;
        }

        /// <summary>The device is open, on the main thread: subscribe to its stream
        /// (<c>((Sr04Device)device).Measurements()</c>) and start it. Drain that queue in Update() too.</summary>
        protected virtual void OnOpened(DepzDevice device)
        {
        }

        /// <summary>One device event (contract 07: disconnect, hardware fault, text, …), on the main thread.</summary>
        protected virtual void OnEvent(DeviceEvent ev)
        {
            switch (ev)
            {
                case DisconnectedEvent d:
                    Debug.LogWarning($"DEPZ: disconnected ({d.Reason})", this);
                    break;
                case UnsolicitedStatusEvent s:
                    Debug.LogWarning($"DEPZ: device status 0x{s.Status:X2}", this);
                    break;
                case TextEvent t:
                    Debug.Log($"DEPZ: {t.Text}", this);
                    break;
            }
        }
    }
}
#endif
