/**
 * Ambient declaration so the optional peer dependency `serialport` typechecks
 * when it is not installed. NodeSerialTransport narrows the shape it uses via
 * its own local interfaces.
 */
declare module "serialport";
