public class Thrower {
  public static final IllegalStateException error = new IllegalStateException("thrown from java");

  public static native void thrower();

  public static native void rethrower();

  public static void fail() {
    throw error;
  }

  public static void failWithoutMessage() {
    throw new IllegalStateException();
  }

  public static void propagate() {
    thrower();
  }

  public static String caught() {
    try {
      thrower();
    } catch (Throwable e) {
      return e.getClass().getName() + ": " + e.getMessage();
    }

    return null;
  }

  public static boolean roundTrip() {
    try {
      rethrower();
    } catch (IllegalStateException e) {
      return e == error;
    }

    return false;
  }
}
