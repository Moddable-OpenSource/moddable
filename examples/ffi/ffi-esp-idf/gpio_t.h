typedef struct {
    uint32_t fun_sel;               /*!< Value of IOMUX function selection */
    uint32_t sig_out;               /*!< Index of the outputting peripheral signal */
    int32_t drv;                    /*!< Value of drive strength */
    bool pu;                        /*!< Status of pull-up enabled or not */
    bool pd;                        /*!< Status of pull-down enabled or not */
    bool ie;                        /*!< Status of input enabled or not */
    bool oe;                        /*!< Status of output enabled or not */
    bool oe_ctrl_by_periph;         /*!< True if use output enable signal from peripheral, otherwise False */
    bool oe_inv;                    /*!< Whether the output enable signal is inversed or not */
    bool od;                        /*!< Status of open-drain enabled or not */
    bool slp_sel;                   /*!< Status of pin sleep mode enabled or not */
} gpio_io_config_t;
