package com.github.andreasarvidsson.windsp.config;

import javax.ws.rs.ApplicationPath;

/**
 *
 * @author Andreas Arvidsson
 */
@ApplicationPath("rest")
public class Application extends javax.ws.rs.core.Application {

    public Application() {
        System.out.println("Starting application");
        // Throw error on starup if the config file doesn't exist.
        ConfFile.get();
    }
}
