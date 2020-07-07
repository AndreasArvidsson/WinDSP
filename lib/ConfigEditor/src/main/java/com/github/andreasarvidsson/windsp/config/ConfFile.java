package com.github.andreasarvidsson.windsp.config;

import java.io.File;
import javax.ws.rs.NotFoundException;

/**
 *
 * @author Andreas Arvidsson
 */
public abstract class ConfFile {

    private static File file = null;

    public static File get() {
        if (file == null) {
            String configFilePath = System.getProperty("conf");
            if (configFilePath == null) {
                configFilePath = "WinDSP.json";
            }
            file = new File(configFilePath);
            if (!file.exists()) {
                throw new NotFoundException(String.format(
                        "Can't find config file '%s'. To run with another config file use parameter: -Dconf=CONFIG_PATH",
                        file.getAbsolutePath()
                ));
            }
            System.out.printf("Using config file: %s\n", file.getAbsolutePath());
        }
        return file;
    }

}
