#if !NET5_0_OR_GREATER
namespace System.Runtime.CompilerServices;

// netstandard2.1 (Unity) lacks this marker type; the compiler needs it for `record` and `init`.
internal static class IsExternalInit { }
#endif
