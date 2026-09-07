package com.indedge.terminal.app.ui.adapters

import android.annotation.SuppressLint
import android.annotation.SuppressLint
import android.bluetooth.BluetoothDevice
import android.content.Context
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.ArrayAdapter
import android.widget.TextView
import com.indedge.terminal.app.R

/**
 * 经典蓝牙设备列表适配器（名称 + 地址，标注配对/连接状态）。
 */
@SuppressLint("MissingPermission")
@SuppressLint("MissingPermission")
class BtDeviceAdapter(context: Context) :
    ArrayAdapter<BluetoothDevice>(context, R.layout.item_device, mutableListOf()) {

    /** 当前 SPP 已连接设备的地址（命中项显示"已连接"标记） */
    var connectedAddress: String? = null

    override fun getView(position: Int, convertView: View?, parent: ViewGroup): View {
        val view = convertView ?: LayoutInflater.from(context)
            .inflate(R.layout.item_device, parent, false)
        val d = getItem(position) ?: return view
        val bonded = d.bondState == BluetoothDevice.BOND_BONDED
        val isConnected = connectedAddress != null && connectedAddress == d.address
        val nameBase = d.name ?: context.getString(R.string.unknown_device)
        view.findViewById<TextView>(R.id.tvDeviceName).text = if (isConnected) {
            context.getString(R.string.bt_item_connected, nameBase)
        } else {
            nameBase
        }
        view.findViewById<TextView>(R.id.tvDeviceAddr).text = context.getString(
            R.string.bt_item_addr, d.address,
            if (isConnected) context.getString(R.string.connected)
            else if (bonded) context.getString(R.string.bonded)
            else context.getString(R.string.unbonded)
        )
        return view
    }
}
