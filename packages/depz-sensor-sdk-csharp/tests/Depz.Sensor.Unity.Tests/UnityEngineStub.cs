// The slice of UnityEngine that unity/Runtime/DepzSensorBehaviour.cs uses, so the component
// compiles and runs outside Unity; Debug records what it is given.
namespace UnityEngine;

public class MonoBehaviour
{
}

[AttributeUsage(AttributeTargets.Field)]
public sealed class TooltipAttribute : Attribute
{
    public TooltipAttribute(string tooltip) => Tooltip = tooltip;

    public string Tooltip { get; }
}

public static class Debug
{
    public static readonly List<string> Lines = new();

    public static void Log(object message, object? context = null) => Lines.Add("log: " + message);

    public static void LogWarning(object message, object? context = null) => Lines.Add("warning: " + message);

    public static void LogError(object message, object? context = null) => Lines.Add("error: " + message);

    public static void LogException(Exception exception, object? context = null) => Lines.Add("exception: " + exception.Message);
}
