package com.indedge.terminal.app.ui.adapters

import android.content.Context
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.ArrayAdapter
import android.widget.TextView
import com.indedge.terminal.app.R
import com.indedge.terminal.app.ble.BleDevice

/**
 * BLE 扫描设备列表适配器（名称 + 地址 + 信号强度）。
 */
class BleDeviceAdapter(context: Context) :
    ArrayAdapter<BleDevice>(context, R.layout.item_device, mutableListOf()) {

    override fun getView(position: Int, convertView: View?, parent: ViewGroup): View {
        val view = convertView ?: LayoutInflater.from(context)
            .inflate(R.layout.item_device, parent, false)
        val d = getItem(position) ?: return view
        view.findViewById<TextView>(R.id.tvDeviceName).text =
            context.getString(R.string.ble_item_name, d.name, d.rssi)
        view.findViewById<TextView>(R.id.tvDeviceAddr).text = d.device.address
        return view
    }
}
