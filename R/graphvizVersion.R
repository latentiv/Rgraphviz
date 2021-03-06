graphvizVersion <- function() {
    installed_version <- numeric_version(.Call("Rgraphviz_graphvizVersion", PACKAGE = "Rgraphviz"))
    build_version <- numeric_version(.graphviz_build_version$version)
    bundled <- as.logical(.graphviz_build_version$bundled)
    list(installed_version = installed_version, build_version = build_version, bundled_graphviz = bundled)
}

graphvizCapabilities <- function() {
    if(graphvizVersion()$installed_version < numeric_version("2.28.0")) {
        warning("'graphvizCapabilities' needs graphviz >= 2.28.0 to be precise")
        capabilities <- list(layoutTypes = c("dot", "neato", "twopi", "circo", "fdp"))
    } else {
        capabilities <- .Call("Rgraphviz_capabilities", PACKAGE = "Rgraphviz")
    }
    layoutTypes <- capabilities$layoutTypes
    if(graphvizVersion()$bundled_graphviz) {
        ## These do not currently work for 2.28.0
        layoutTypes <- setdiff(layoutTypes, c("sfdp", "nop", "nop1", "nop2", "patchwork"))
    }
    if("dot" %in% layoutTypes)
        layoutTypes <- c("dot", layoutTypes[layoutTypes != "dot"])
    capabilities$layoutTypes <- layoutTypes
    deviceTypes <- capabilities$deviceTypes
    if(graphvizVersion()$bundled_graphviz) {
        ## These do not currently work for 2.28.0
        deviceTypes <- setdiff(deviceTypes, c("svgz", "vmlz"))
    }
    capabilities$deviceTypes <- deviceTypes
    capabilities
}

.checkLayoutType <- function(layoutType) {
    if(layoutType %in% graphvizCapabilities()$layoutTypes)
        return(layoutType)
    else
        stop(sprintf("The value of 'layoutType' ('%s') is not supported.  See 'graphvizCapabilities()$layoutTypes' for supported layouts",
                     layoutType))
}

.checkFileType <- function(fileType) {
    if(fileType %in% graphvizCapabilities()$deviceTypes)
        return(fileType)
    else
        stop(sprintf("The value of 'fileType' ('%s') is not supported.  See 'graphvizCapabilities()$deviceTypes' for supported file types",
                     fileType))
}

        
