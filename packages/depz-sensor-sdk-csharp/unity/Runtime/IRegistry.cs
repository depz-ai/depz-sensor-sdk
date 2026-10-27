#nullable enable
using System;
using System.Collections.Generic;
using System.IO;
using System.Security;
using Microsoft.Win32;

namespace Depz.Sensor.Unity
{
    /// <summary>
    /// The slice of the Windows registry port discovery reads, under
    /// HKEY_LOCAL_MACHINE. A missing or unreadable key is null, never an
    /// exception, so discovery degrades to "ports without USB ids" instead of
    /// failing. <see cref="WindowsRegistry"/> is the real one; tests supply a fake.
    /// </summary>
    public interface IRegistry
    {
        /// <summary>The subkey names of <paramref name="path"/>, or null when the key is missing or unreadable.</summary>
        IReadOnlyList<string>? SubKeyNames(string path);

        /// <summary>The value names of <paramref name="path"/>, or null when the key is missing or unreadable.</summary>
        IReadOnlyList<string>? ValueNames(string path);

        /// <summary>The string value <paramref name="name"/> of <paramref name="path"/>, or null when the key
        /// or the value is missing, unreadable, or not a string.</summary>
        string? GetString(string path, string name);
    }

    /// <summary>HKEY_LOCAL_MACHINE through Microsoft.Win32.Registry. Off Windows every key is missing.</summary>
    public sealed class WindowsRegistry : IRegistry
    {
        /// <summary>True on Windows, where the registry exists.</summary>
        public static bool IsAvailable => Environment.OSVersion.Platform == PlatformID.Win32NT;

        public IReadOnlyList<string>? SubKeyNames(string path) => Read(path, k => k.GetSubKeyNames());

        public IReadOnlyList<string>? ValueNames(string path) => Read(path, k => k.GetValueNames());

        public string? GetString(string path, string name) => Read(path, k => k.GetValue(name) as string);

        private static T? Read<T>(string path, Func<RegistryKey, T?> read) where T : class
        {
            if (!IsAvailable)
                return null;
            try
            {
                using (RegistryKey? key = Registry.LocalMachine.OpenSubKey(path))
                    return key is null ? null : read(key);
            }
            catch (Exception e) when (e is SecurityException || e is UnauthorizedAccessException || e is IOException
                                      || e is ObjectDisposedException)
            {
                return null;
            }
        }
    }
}
