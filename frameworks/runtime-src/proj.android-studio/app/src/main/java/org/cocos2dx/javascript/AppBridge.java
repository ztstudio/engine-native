package org.cocos2dx.javascript;

import android.app.ActivityManager;
import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.Context;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.res.Resources;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.os.Build;
import android.text.TextUtils;
import android.util.DisplayMetrics;
import android.util.Log;

import org.cocos2dx.lib.Cocos2dxHelper;
import org.cocos2dx.lib.Cocos2dxJavascriptJavaBridge;
import org.json.JSONException;
import org.json.JSONObject;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.IOException;
import java.net.NetworkInterface;
import java.net.SocketException;
import java.util.Enumeration;
import java.util.UUID;

public class AppBridge {
    private static final String TAG = "AppBridge";
    private static JSONObject deviceInfo = null;
    private static String cpExtProps = "";
    /**
     * The default return value of any method in this class when an
     * error occurs or when processing fails (Currently set to -1). Use this to check if
     * the information about the device in question was successfully obtained.
     */
    public static final int DEVICEINFO_UNKNOWN = -1;


    public static String getRes(String name) {
        Context context = Cocos2dxHelper.getActivity();
        Resources resources = AppActivity.getContext().getResources();
        int resId = resources.getIdentifier(name, "string", context.getPackageName());
        return resId != 0 ? resources.getString(resId) : "";
    }

    public static void runScript(final String eval) {
        Log.i(TAG, "runScript: " + eval);
        Cocos2dxHelper.runOnGLThread(() -> {
            int result = Cocos2dxJavascriptJavaBridge.evalString(eval);
            Log.i(TAG, "runScript result: " + result);
        });
    }

    public static String getCPExtProps() {
        return cpExtProps;
    }

    public static void initCPExtProps() {
//        JSONObject json = new JSONObject();
//        try {
//            json.put("locale", getRes("locale"));
//            json.put("channel", getRes("channel"));
//        } catch (JSONException ignored) {
//        }
//        cpExtProps = json.toString();
//        Log.i(TAG, cpExtProps);
    }

    public static void hideSplash()
    {
//        ((AppActivity)Cocos2dxHelper.getActivity()).hideSplash();
    }

    public static void setHotVersion(String version) {
//        CrashReport.setAppVersion(Cocos2dxHelper.getActivity(), version);
    }

    public static String getClipboard() {
        Context context = Cocos2dxHelper.getActivity();
        ClipboardManager clipboardManager = (ClipboardManager) context.getSystemService(Context.CLIPBOARD_SERVICE);
        String result = "";
        if (clipboardManager != null && clipboardManager.hasPrimaryClip()) {
            try {
                ClipData.Item clipData = clipboardManager.getPrimaryClip().getItemAt(0);
                CharSequence sequence = clipData.getText();
                if (sequence != null)
                    result = sequence.toString();
            } catch (NullPointerException e) {
                Log.e(TAG, "访问剪贴板失败");
            }
        }
        return result;
    }

    public static String getAABPath() {
        return "raw-assets";
    }

    public static void reportException(String location, String message, String stack) {
//        CrashReport.postException(4, location, message, stack, null);
    }

    // 获取设备信息
    public static String getDeviceInfo() throws JSONException {
        Context context = Cocos2dxHelper.getActivity();
        if (deviceInfo == null) {
            JSONObject info = deviceInfo = new JSONObject();
            String packageId = context.getPackageName();
            String clientVersion = getVersion();
            String deviceId = getUniquePseudoID();
            String deviceModel = getDeviceName();
            String os = "Android";
            String osVersion = Build.VERSION.RELEASE;
            String sdkVersion = "" + Build.VERSION.SDK_INT;
            String resolution = getResolution();
            Boolean isRooted = isRooted();
            String mac = getMac();
            info.put("package", packageId);
            info.put("clientVersion", clientVersion);
            info.put("deviceId", deviceId);
            info.put("deviceModel", deviceModel);
            info.put("os", os);
            info.put("osVersion", osVersion);
            info.put("sdkVersion", sdkVersion);
            info.put("resolution", resolution);
            info.put("isRooted", isRooted);
            info.put("mac", mac);
        }
        deviceInfo.put("network", getNetwork());
        deviceInfo.put("battery", getBattery());
        return deviceInfo.toString();
    }

    public static String getNetwork() {
        int type = Cocos2dxHelper.getNetworkType();
        if (type == Cocos2dxHelper.NETWORK_TYPE_LAN) {
            return "wifi";
        } else if (type == Cocos2dxHelper.NETWORK_TYPE_WWAN) {
            return "mobile";
        } else {
            return "other";
        }
    }

    public static float getBattery() {
        return Cocos2dxHelper.getBatteryLevel();
    }

    private static String getResolution() {
        DisplayMetrics dm = Cocos2dxHelper.getActivity().getResources().getDisplayMetrics();
        return dm.widthPixels + "x" + dm.heightPixels;
    }

    private static boolean findBinary(String binaryName) {
        boolean found = false;
        String[] places = {"/sbin/", "/system/bin/", "/system/xbin/",
                "/data/local/xbin/", "/data/local/bin/",
                "/system/sd/xbin/", "/system/bin/failsafe/", "/data/local/"};
        for (String where : places) {
            if (new File(where + binaryName).exists()) {
                found = true;

                break;
            }
        }
        return found;
    }

    private static boolean isRooted() {
        return findBinary("su");
    }

    public static String getVersion() {
        Context context = Cocos2dxHelper.getActivity();
        PackageManager packageManager = context.getPackageManager();
        try {
            PackageInfo info = packageManager.getPackageInfo(context.getPackageName(), 0);
            return info.versionName;
        } catch (PackageManager.NameNotFoundException e) {
            e.printStackTrace();
        }
        return "";
    }

    public static String getVersionName(){
        return "";
    }

    private static String getDeviceName() {
        String manufacturer = Build.MANUFACTURER;
        String model = Build.MODEL;
        if (model.toLowerCase().startsWith(manufacturer.toLowerCase())) {
            return capitalize(model);
        } else {
            return capitalize(manufacturer) + " " + model;
        }
    }

    private static String capitalize(String s) {
        if (s == null || s.length() == 0) {
            return "";
        }
        char first = s.charAt(0);
        if (Character.isUpperCase(first)) {
            return s;
        } else {
            return Character.toUpperCase(first) + s.substring(1);
        }
    }

    //获得独一无二的Pseudo ID
    private static String getUniquePseudoID() {
        String serial;
        String m_szDevIDShort = "35" +
                Build.BOARD.length() % 10 + Build.BRAND.length() % 10 +
                Build.CPU_ABI.length() % 10 + Build.DEVICE.length() % 10 +
                Build.DISPLAY.length() % 10 + Build.HOST.length() % 10 +
                Build.ID.length() % 10 + Build.MANUFACTURER.length() % 10 +
                Build.MODEL.length() % 10 + Build.PRODUCT.length() % 10 +
                Build.TAGS.length() % 10 + Build.TYPE.length() % 10 +
                Build.USER.length() % 10; //13 位
        try {
            serial = android.os.Build.class.getField("SERIAL").get(null).toString();
            //API>=9 使用serial号
            return new UUID(m_szDevIDShort.hashCode(), serial.hashCode()).toString();
        } catch (Exception exception) {
            //serial需要一个初始化
            serial = "serial"; // 随便一个初始化
        }
        //使用硬件信息拼凑出来的15位号码
        return new UUID(m_szDevIDShort.hashCode(), serial.hashCode()).toString();
    }

    /**
     * 获取wifi的mac地址，适配到android Q
     *
     * @return mac地址
     */
    public static String getMac() {
        Context paramContext = Cocos2dxHelper.getActivity();
        try {
            if (Build.VERSION.SDK_INT >= 23) {
                String str = getMacMoreThanM(paramContext);
                if (!TextUtils.isEmpty(str))
                    return str;
            }
            // 6.0以下手机直接获取wifi的mac地址即可
            WifiManager wifiManager = (WifiManager) paramContext.getApplicationContext().getSystemService(Context.WIFI_SERVICE);
            WifiInfo wifiInfo = wifiManager.getConnectionInfo();
            if (wifiInfo != null)
                return wifiInfo.getMacAddress();
        } catch (Throwable ignored) {
        }
        return "";
    }

    /**
     * android 6.0+获取wifi的mac地址
     */
    private static String getMacMoreThanM(Context paramContext) {
        try {
            //获取本机器所有的网络接口
            Enumeration enumeration = NetworkInterface.getNetworkInterfaces();
            while (enumeration.hasMoreElements()) {
                NetworkInterface networkInterface = (NetworkInterface) enumeration.nextElement();
                //获取硬件地址，一般是MAC
                byte[] arrayOfByte = networkInterface.getHardwareAddress();
                if (arrayOfByte == null || arrayOfByte.length == 0) {
                    continue;
                }

                StringBuilder stringBuilder = new StringBuilder();
                for (byte b : arrayOfByte) {
                    //格式化为：两位十六进制加冒号的格式，若是不足两位，补0
                    stringBuilder.append(String.format("%02X:", b));
                }
                if (stringBuilder.length() > 0) {
                    //删除后面多余的冒号
                    stringBuilder.deleteCharAt(stringBuilder.length() - 1);
                }
                String str = stringBuilder.toString();
                // wlan0:无线网卡 eth0：以太网卡
                if (networkInterface.getName().equals("wlan0")) {
                    return str;
                }
            }
        } catch (SocketException ignored) {
        }
        return "";
    }

    /**
     * 获取埋点信息
     */
    public static String getAnalyticsProps() {
        return "";
    }

    /**
     * 上传埋点信息
     */
    public static void uploadAnalytics(String json) {

    }

    //获取RAM容量
    public static int getMemoryMB() {
        int memInMB = 2048;
        Context c = Cocos2dxHelper.getActivity();
        // memInfo.totalMem not supported in pre-Jelly Bean APIs.
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN) {
            ActivityManager.MemoryInfo memInfo = new ActivityManager.MemoryInfo();
            ActivityManager am = (ActivityManager) c.getSystemService(Context.ACTIVITY_SERVICE);
            am.getMemoryInfo(memInfo);
            if (memInfo != null) {
                memInMB = (int) (memInfo.totalMem / (1024 * 1024));
            }
        } else {
            try {
                FileInputStream stream = new FileInputStream("/proc/meminfo");
                try {
                    int memInKB = parseFileForValue("MemTotal", stream);
                    if (memInKB != DEVICEINFO_UNKNOWN) {
                        memInMB = memInKB / 1024;
                    }
                } finally {
                    stream.close();
                }
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
        return memInMB;
    }

    //获取CPU型号
    public static String getCPUName() {
        try {
            FileReader fr = new FileReader("/proc/cpuinfo");
            BufferedReader br = new BufferedReader(fr);
            String text;
            String last = "";
            while ((text = br.readLine()) != null) {
                last = text;
            }
            //一般机型的cpu型号都会在cpuinfo文件的最后一行
            if (last.contains("Hardware")) {
                String[] hardWare = last.split(":\\s+", 2);
                return hardWare[1];
            }
        } catch (FileNotFoundException e) {
            e.printStackTrace();
        } catch (IOException e) {
            e.printStackTrace();
        }
        return Build.HARDWARE;
    }

    /**
     * Helper method for reading values from system files, using a minimised buffer.
     *
     * @param textToMatch - Text in the system files to read for.
     * @param stream      - FileInputStream of the system file being read from.
     * @return A numerical value following textToMatch in specified the system file.
     * -1 in the event of a failure.
     */
    private static int parseFileForValue(String textToMatch, FileInputStream stream) {
        byte[] buffer = new byte[1024];
        try {
            int length = stream.read(buffer);
            for (int i = 0; i < length; i++) {
                if (buffer[i] == '\n' || i == 0) {
                    if (buffer[i] == '\n') i++;
                    for (int j = i; j < length; j++) {
                        int textIndex = j - i;
                        //Text doesn't match query at some point.
                        if (buffer[j] != textToMatch.charAt(textIndex)) {
                            break;
                        }
                        //Text matches query here.
                        if (textIndex == textToMatch.length() - 1) {
                            return extractValue(buffer, j);
                        }
                    }
                }
            }
        } catch (IOException e) {
            //Ignore any exceptions and fall through to return unknown value.
        } catch (NumberFormatException e) {
        }
        return DEVICEINFO_UNKNOWN;
    }

    /**
     * Helper method used by {@link #parseFileForValue(String, FileInputStream) parseFileForValue}. Parses
     * the next available number after the match in the file being read and returns it as an integer.
     * @param index - The index in the buffer array to begin looking.
     * @return The next number on that line in the buffer, returned as an int. Returns
     * DEVICEINFO_UNKNOWN = -1 in the event that no more numbers exist on the same line.
     */
    private static int extractValue(byte[] buffer, int index) {
        while (index < buffer.length && buffer[index] != '\n') {
            if (Character.isDigit(buffer[index])) {
                int start = index;
                index++;
                while (index < buffer.length && Character.isDigit(buffer[index])) {
                    index++;
                }
                String str = new String(buffer, 0, start, index - start);
                return Integer.parseInt(str);
            }
            index++;
        }
        return DEVICEINFO_UNKNOWN;
    }
}
