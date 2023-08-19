local skynet = require 'skynet'

function beelog_info(...)
    skynet.error("BEEI", ...)
end

function beelog_warning(...)
    skynet.error("BEEW", ...)
end

function beelog_error(...)
    skynet.error("BEEE", ...)
end